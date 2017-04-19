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
import android.os.Handler;
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
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
import java.util.Random;
import java.util.UUID;


public class ControlActivity extends AppCompatActivity {
    private final List<String> tail_modes = Arrays.asList(
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

    private final List<String> eye_modes = Arrays.asList(
            "Off",
            "Fixed",
            "Strobe",
            "Fade",
            "Two-Color",
            "Rainbow",
            "Blacklight"
    );

    private class ComponentConfig {
        public final short magic = 0x7f7f;
        public short mode = 0;
        public int primary_color = Color.RED;
        public int secondary_color = Color.CYAN;
    }

    ComponentConfig tail = new ComponentConfig();
    ComponentConfig eyes = new ComponentConfig();

    private final int TAIL_COLOR_REQUEST = 1;
    private final int EYES_COLOR_REQUEST = 2;

    public final UUID CostumeSvcID = UUID.fromString(
            "02AE007F-02AE-02AE-02AE-02AE02AE02AE"
    );
    public final UUID CostumeCTLChangedCharID = UUID.fromString(
            "02AE017F-02AE-02AE-02AE-02AE02AE02AE"
    );
    public final UUID CostumeCTLFreqPercCharID = UUID.fromString(
            "02AE027F-02AE-02AE-02AE-02AE02AE02AE"
    );
    public final UUID CostumeCTLTailCharID = UUID.fromString(
            "02AE037F-02AE-02AE-02AE-02AE02AE02AE"
    );
    public final UUID CostumeCTLEyesCharID = UUID.fromString(
            "02AE047F-02AE-02AE-02AE-02AE02AE02AE"
    );

    final protected static char[] hexArray = "0123456789ABCDEF".toCharArray();
    public static String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        for ( int j = 0; j < bytes.length; j++ ) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0F];
        }
        return new String(hexChars);
    }


    private BluetoothGatt mGatt;
    private BluetoothDevice device;

    public void connectToDevice(BluetoothDevice device) {
        mGatt = device.connectGatt(this, false, gattCallback);
        // FIXME the following is a total hack...
        Handler handler = new Handler();
        handler.postDelayed(new Runnable() {

            @Override
            public void run() {
                mGatt.disconnect();
            }
        }, 8000); // 1000ms delay
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        private byte changed = 3;
        private Queue wq = new LinkedList();
        private boolean wActive = false;

        private void writeChr(BluetoothGattCharacteristic chr) {
            wq.add(chr);
            writeNextValueFromQueue();
        }

        private void writeNextValueFromQueue() {
            BluetoothGattCharacteristic chr;
            if (wActive) {
                return;
            }
            if (wq.size() == 0) {
                return;
            }
            wActive = true;

            chr = (BluetoothGattCharacteristic)wq.poll();

            mGatt.writeCharacteristic(chr);
        }

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
            BluetoothGattCharacteristic chr;
            final SeekBar sb = (SeekBar) findViewById(R.id.freq_bar);
            BluetoothGattService svc = mGatt.getService(CostumeSvcID);

            ByteBuffer msg_buf;

            // Write freqperc
            byte freqperc = (byte)sb.getProgress();
            chr = svc.getCharacteristic(CostumeCTLFreqPercCharID);
            chr.setValue(freqperc, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
            writeChr(chr);

            // Write tail
            msg_buf = prepareTailBuf();
            chr = svc.getCharacteristic(CostumeCTLTailCharID);
            Log.i("Writing tail:", bytesToHex(msg_buf.array()));
            chr.setValue(msg_buf.array());
            writeChr(chr);

            // Write eyes
            msg_buf = prepareEyesBuf();
            chr = svc.getCharacteristic(CostumeCTLEyesCharID);
            Log.i("Writing eyes:", bytesToHex(msg_buf.array()));
            chr.setValue(msg_buf.array());
            writeChr(chr);

            // FIXME should increment actual changed value, but a incrementing hack works for now...
            // Write changed
            changed++;
            final Random rand = new Random();
            changed = (byte)rand.nextInt();
            chr = svc.getCharacteristic(CostumeCTLChangedCharID);
            chr.setValue(changed, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
            writeChr(chr);
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
            wActive = false;
            writeNextValueFromQueue();

            Log.e("Writing char: ", characteristic.getUuid().toString());
        }
    };

    private ByteBuffer prepareTailBuf() {
        ByteBuffer msg_buf = ByteBuffer.allocate(12);
        msg_buf.clear();
        msg_buf.order(ByteOrder.LITTLE_ENDIAN);
        msg_buf.putShort(tail.magic);

        msg_buf.putShort(tail.mode);
        msg_buf.putInt(color_convert_rgb_lpd8806(tail.primary_color));
        msg_buf.putInt(color_convert_rgb_lpd8806(tail.secondary_color));

        return msg_buf;
    }

    private ByteBuffer prepareEyesBuf() {
        ByteBuffer msg_buf = ByteBuffer.allocate(12);
        msg_buf.clear();
        msg_buf.order(ByteOrder.LITTLE_ENDIAN);
        msg_buf.putShort(eyes.magic);

        msg_buf.putShort(eyes.mode);
        msg_buf.putInt(color_convert_rgb_np(eyes.primary_color));
        msg_buf.putInt(color_convert_rgb_np(eyes.secondary_color));

        return msg_buf;
    }

    private void sendMessage() {
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

    /**
     * Convert a native 24bpp rgb value to the 24bpp grb format used by the NeoPixel LEDs
     * @param color 24bpp rgb color
     * @return 24bpp grb color equivalent
     */
    private static int color_convert_rgb_np(int color)
    {
        int r, g, b;
        r = color >> 16 & 0xFF;
        g = color >> 8 & 0xFF;
        b = color & 0xFF;
        return (g << 16 | r << 8 | b);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // TODO: read attributes from device, set settings accordingly.
        setContentView(R.layout.activity_main);
        final Context ctx = this;

        final Spinner tail_mode_select = (Spinner) findViewById(R.id.tail_mode_select);
        ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, tail_modes);
        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        tail_mode_select.setAdapter(dataAdapter);

        tail_mode_select.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                tail.mode = (byte) position;
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                /* Do nothing */
            }
        });

        final Spinner eyes_mode_select = (Spinner) findViewById(R.id.eyes_mode_select);
        ArrayAdapter<String> eyesdataAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, eye_modes);
        eyesdataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        eyes_mode_select.setAdapter(eyesdataAdapter);

        eyes_mode_select.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                eyes.mode = (byte) position;
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

        final SeekBar sb = (SeekBar) findViewById(R.id.freq_bar);
        sb.setProgress(50);

        final SurfaceView tail_primary_sv = (SurfaceView) findViewById(R.id.tail_primary_sv);
        final SurfaceView tail_secondary_sv = (SurfaceView) findViewById(R.id.tail_secondary_sv);
        tail_primary_sv.getHolder().setFixedSize(150, 150);
        tail_primary_sv.setBackgroundColor(tail.primary_color);
        tail_secondary_sv.getHolder().setFixedSize(150, 150);
        tail_secondary_sv.setBackgroundColor(tail.secondary_color);

        tail_primary_sv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ctx, ColorSelectorActivity.class);
                intent.putExtra("cv_index", 0);
                intent.putExtra("color", tail.primary_color);
                startActivityForResult(intent, TAIL_COLOR_REQUEST);
            }
        });

        tail_secondary_sv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ctx, ColorSelectorActivity.class);
                intent.putExtra("cv_index", 1);
                intent.putExtra("color", tail.secondary_color);
                startActivityForResult(intent, TAIL_COLOR_REQUEST);
            }
        });

        final SurfaceView eyes_primary_sv = (SurfaceView) findViewById(R.id.eyes_primary_sv);
        final SurfaceView eyes_secondary_sv = (SurfaceView) findViewById(R.id.eyes_secondary_sv);
        eyes_primary_sv.getHolder().setFixedSize(150, 150);
        eyes_primary_sv.setBackgroundColor(eyes.primary_color);
        eyes_secondary_sv.getHolder().setFixedSize(150, 150);
        eyes_secondary_sv.setBackgroundColor(eyes.secondary_color);

        eyes_primary_sv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ctx, ColorSelectorActivity.class);
                intent.putExtra("cv_index", 0);
                intent.putExtra("color", eyes.primary_color);
                startActivityForResult(intent, EYES_COLOR_REQUEST);
            }
        });

        eyes_secondary_sv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ctx, ColorSelectorActivity.class);
                intent.putExtra("cv_index", 1);
                intent.putExtra("color", eyes.secondary_color);
                startActivityForResult(intent, EYES_COLOR_REQUEST);
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
        int color, cv_ndx;
        if (resultCode < 0) {
            return;
        }

        switch (requestCode) {
            case TAIL_COLOR_REQUEST:
                color = data.getIntExtra("color", 0);
                cv_ndx = data.getIntExtra("cv_index", 0);

                if (cv_ndx > 2) {
                    return;
                }

                if (cv_ndx == 0) {
                    tail.primary_color = color;
                    findViewById(R.id.tail_primary_sv).setBackgroundColor(color);
                }
                else if (cv_ndx == 1) {
                    tail.secondary_color = color;
                    findViewById(R.id.tail_secondary_sv).setBackgroundColor(color);
                }
                break;
            case EYES_COLOR_REQUEST:

                color = data.getIntExtra("color", 0);
                cv_ndx = data.getIntExtra("cv_index", 0);

                if (cv_ndx > 2) {
                    return;
                }

                if (cv_ndx == 0) {
                    eyes.primary_color = color;
                    findViewById(R.id.eyes_primary_sv).setBackgroundColor(color);
                }
                else if (cv_ndx == 1) {
                    eyes.secondary_color = color;
                    findViewById(R.id.eyes_secondary_sv).setBackgroundColor(color);
                }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }
}
