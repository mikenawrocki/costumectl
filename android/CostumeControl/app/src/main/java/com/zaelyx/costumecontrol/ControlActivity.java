package com.zaelyx.costumecontrol;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.Spinner;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;


public class ControlActivity extends AppCompatActivity {
    private final List<String> modes = Arrays.asList(
            "Off",
            "Fixed",
            "Strobe",
            "Fade",
            "Two-Color",
            "Top-Down",
            "Bottom-Up",
            "Rainbow",
            "Bi-Pride"
    );
    private String mode = modes.get(0);
    private int[] colors = {Color.RED, Color.CYAN};

    public final UUID CostumeSvcID = UUID.fromString("02AE007F-02AE-02AE-02AE-02AE02AE02AE");
    public final UUID CostumeCTLCharID = UUID.fromString("02AE017F-02AE-02AE-02AE-02AE02AE02AE");
    public final UUID CostumeStatusCharID = UUID.fromString("02AE027F-02AE-02AE-02AE-02AE02AE02AE");

    private BluetoothGatt mGatt;
    private BluetoothDevice device;

    ByteBuffer msg_buf = ByteBuffer.allocate(12);

    public void connectToDevice(BluetoothDevice device) {
        mGatt = device.connectGatt(this, false, gattCallback);
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            Log.i("onConnectionStateChange", "Status: " + status);
            switch (newState) {
                case BluetoothProfile.STATE_CONNECTED:
                    Log.i("gattCallback", "STATE_CONNECTED");
                    gatt.discoverServices();
                    break;
                case BluetoothProfile.STATE_DISCONNECTED:
                    Log.e("gattCallback", "STATE_DISCONNECTED");
                    mGatt.disconnect();
                    break;
                default:
                    Log.e("gattCallback", "STATE_OTHER");
            }

        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            byte[] msg = msg_buf.array();

            BluetoothGattService svc = mGatt.getService(CostumeSvcID);
            BluetoothGattCharacteristic chr = svc.getCharacteristic(CostumeCTLCharID);
            chr.setValue(msg);
            mGatt.writeCharacteristic(chr);
            Log.i("onServicesDiscovered", "FOO");
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic
                                                 characteristic, int status) {
            Log.i("onCharacteristicRead", characteristic.getStringValue(0));
        }
        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt,
                                          BluetoothGattCharacteristic characteristic,
                                          int status) {
            Log.i("onCharacteristicWrite", Integer.toString(status));
            mGatt.disconnect();
        }
    };


    private void sendMessage() {
        final Spinner mode_select = (Spinner) findViewById(R.id.mode_select);
        final SeekBar sb = (SeekBar) findViewById(R.id.freq_bar);

        /* Build message:
        uint16_t magic; (0x7F7F)
        uint8_t mode;
        uint8_t freq_perc;
        uint32_t primary_color;
        uint32_t secondary_color;
         */

        msg_buf.clear();
        msg_buf.order(ByteOrder.LITTLE_ENDIAN);
        msg_buf.putShort((short)0x7F7F);
        msg_buf.put((byte) modes.indexOf(mode));
        msg_buf.put((byte) sb.getProgress());
        msg_buf.putInt(color_convert_rgb_lpd8806(colors[0]));
        msg_buf.putInt(color_convert_rgb_lpd8806(colors[1]));

        connectToDevice(device);
    }

    /**
     * Convert a native 24bpp rgb value to the 21bpp grb format used by the LPD8806 LED strip
     * @param color 24bpp rgb color
     * @return 21bpp grb color equivalent
     */
    private static int color_convert_rgb_lpd8806(int color)
    {
        byte r, g, b;
        r = (byte) (color >> 17 & 0x7F);
        g = (byte) (color >> 9  & 0x7F);
        b = (byte) (color >> 1  & 0x7F);
        return (g << 16 | r << 8 | b);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);



        // TODO: read attributes from device, set settings accordingly.
        setContentView(R.layout.activity_main);
        final Context ctx = this;

        final Spinner mode_select = (Spinner) findViewById(R.id.mode_select);
        ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, modes);
        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mode_select.setAdapter(dataAdapter);

        mode_select.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                mode = modes.get(position);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                /* Do nothing */
            }
        });

        final Button send_btn = (Button) findViewById(R.id.send_btn);
        send_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                sendMessage();
            }
        });

        final SurfaceView color_one_sv = (SurfaceView) findViewById(R.id.color_one_sv);
        final SurfaceView color_two_sv = (SurfaceView) findViewById(R.id.color_two_sv);

        color_one_sv.getHolder().setFixedSize(200, 200);
        color_one_sv.setBackgroundColor(colors[0]);
        color_two_sv.getHolder().setFixedSize(200, 200);
        color_two_sv.setBackgroundColor(colors[1]);

        color_one_sv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ctx, ColorSelectorActivity.class);
                intent.putExtra("cv_index", 0);
                intent.putExtra("color", colors[0]);
                startActivityForResult(intent, 1);
            }
        });

        color_two_sv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ctx, ColorSelectorActivity.class);
                intent.putExtra("cv_index", 1);
                intent.putExtra("color", colors[1]);
                startActivityForResult(intent, 1);
            }
        });

        Intent rq_intent = getIntent();

        device = rq_intent.getParcelableExtra("ble_device");

        ActionBar ab = getSupportActionBar();
        ab.setDisplayHomeAsUpEnabled(true);
        ab.setTitle(device.getName());
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        SurfaceView[] svs = {
                (SurfaceView) findViewById(R.id.color_one_sv),
                (SurfaceView) findViewById(R.id.color_two_sv)
        };

        switch (requestCode) {
            case 1:
                if (resultCode < 0) {
                    break;
                }
                int color = data.getIntExtra("color", 0);
                int cv_ndx = data.getIntExtra("cv_index", 0);
                if (cv_ndx > svs.length) {
                    break;
                }
                colors[cv_ndx] = color;
                svs[cv_ndx].setBackgroundColor(color);
        }

        super.onActivityResult(requestCode, resultCode, data);
    }
}
