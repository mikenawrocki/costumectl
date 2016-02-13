package com.zaelyx.costumecontrol;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import com.larswerkman.holocolorpicker.ColorPicker;
import com.larswerkman.holocolorpicker.SVBar;

public class ColorSelectorActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_color_selector);

        final Intent rq_intent = getIntent();

        final ColorPicker picker = (ColorPicker) findViewById(R.id.picker);
        SVBar svBar = (SVBar) findViewById(R.id.svbar);
        picker.addSVBar(svBar);
        picker.setShowOldCenterColor(false);
        int rq_color = rq_intent.getIntExtra("color", 0);
        picker.setColor(rq_color);

        Button ok_btn = (Button) findViewById(R.id.color_ok_btn);
        Button cancel_btn = (Button) findViewById(R.id.color_cancel_btn);

        ok_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent();
                intent.putExtra("color", picker.getColor());
                intent.putExtra("cv_index", rq_intent.getIntExtra("cv_index", 0));
                setResult(1, intent);
                finish();
            }
        });

        cancel_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setResult(-1);
                finish();
            }
        });
    }
}
