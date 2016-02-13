package com.zaelyx.costumecontrol;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.ParcelUuid;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class DeviceChooserActivity extends AppCompatActivity {

    public final UUID CostumeSvcID = UUID.fromString("02AE007F-02AE-02AE-02AE-02AE02AE02AE");

    private class FriendlyBluetoothDevice {
        BluetoothDevice device;

        FriendlyBluetoothDevice(BluetoothDevice dev) {
            this.device = dev;
        }

        @Override
        public String toString() {
            return device.getName();
        }

        @Override
        public boolean equals(Object obj) {
            return device.equals(obj);
        }

        @Override
        public int hashCode() {
            return device.hashCode();
        }

        public BluetoothDevice getDevice() {
            return device;
        }
    }


    private BluetoothAdapter mBluetoothAdapter;
    private int REQUEST_ENABLE_BT = 1;
    private Handler mHandler;
    private static final long SCAN_PERIOD = 10000;
    private BluetoothLeScanner mLEScanner;
    private ScanSettings settings;
    private List<ScanFilter> filters;
    private boolean isScanning = true;

    private List<FriendlyBluetoothDevice> discoveredDevices =
                                                        new ArrayList<FriendlyBluetoothDevice>();
    private ArrayAdapter<FriendlyBluetoothDevice> discoveredDevicesAdapter;

    private void toggleScan() {
        final Button scanBtn = (Button) findViewById(R.id.scan_btn);
        isScanning = !isScanning;
        scanLeDevice(isScanning);
        scanBtn.setText((isScanning) ? "Stop Scanning" : "Start Scanning");
    }

    private void scanLeDevice(final boolean enable) {
        if (enable) {
            Toast.makeText(this, "Scanning for Peripherals", Toast.LENGTH_SHORT).show();
            discoveredDevices.clear();
            discoveredDevicesAdapter.notifyDataSetChanged();
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    mLEScanner.stopScan(mScanCallback);
                    final Button scanBtn = (Button) findViewById(R.id.scan_btn);
                    scanBtn.setText("Start Scanning");
                }
            }, SCAN_PERIOD);
            mLEScanner.startScan(filters, settings, mScanCallback);
        } else {
            mLEScanner.stopScan(mScanCallback);
        }
    }

    private ScanCallback mScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            Log.i("callbackType", String.valueOf(callbackType));
            Log.i("result", result.toString());
            FriendlyBluetoothDevice btDevice = new FriendlyBluetoothDevice(result.getDevice());

            discoveredDevices.add(btDevice);
            discoveredDevicesAdapter.notifyDataSetChanged();
            Log.i("Name: ", result.getDevice().getName());
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            for (ScanResult sr : results) {
                Log.i("ScanResult - Results", sr.toString());
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.e("Scan Failed", "Error Code: " + errorCode);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_chooser);
        final Context ctx = this;

        ListView scanlist = (ListView) findViewById(R.id.scanlist);
        discoveredDevicesAdapter = new ArrayAdapter<>(this,
                android.R.layout.simple_list_item_1, discoveredDevices);
        scanlist.setAdapter(discoveredDevicesAdapter);
        scanlist.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                Intent intent = new Intent(ctx, ControlActivity.class);
                intent.putExtra("ble_device", discoveredDevices.get(position).getDevice());
                startActivity(intent);
            }
        });

        final Button scanBtn = (Button) findViewById(R.id.scan_btn);
        scanBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleScan();
            }
        });

        mHandler = new Handler();
        final BluetoothManager bluetoothManager =
                (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = bluetoothManager.getAdapter();

        if (mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        } else {
            mLEScanner = mBluetoothAdapter.getBluetoothLeScanner();
            settings = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                    .build();
            filters = new ArrayList<ScanFilter>();
            ScanFilter scf = new ScanFilter.Builder().setServiceUuid(
                    new ParcelUuid(CostumeSvcID)).build();
            filters.add(scf);
            scanLeDevice(true);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == Activity.RESULT_CANCELED) {
                //Bluetooth not enabled.
                finish();
                return;
            }
        }

        super.onActivityResult(requestCode, resultCode, data);
    }
}
