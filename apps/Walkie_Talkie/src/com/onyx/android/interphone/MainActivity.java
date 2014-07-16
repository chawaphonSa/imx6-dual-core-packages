package com.onyx.android.interphone;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends Activity
{
    private static final String TAG = "MainActivity";

    private EditText mTx = null;
    private EditText mRx = null;
    private Button mSetButton;
    private String TX="400.12000";
    private String RX="400.12000";

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mTx = (EditText)findViewById(R.id.tx_freq_textview);
        mRx = (EditText)findViewById(R.id.rx_freq_textview);
        mTx.setText(TX);
        mRx.setText(RX);
        mSetButton = (Button)findViewById(R.id.button_set);

        mSetButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String rx = mRx.getText().toString().trim();
                String tx = mTx.getText().toString().trim();
                // setFreq(tx, rx);
                runShellScript(tx, rx);

            }
        });
    }

    private void setFreq(String tx, String rx ) {
        try {
            Runtime r = Runtime.getRuntime();
            r.exec("/system/xbin/su");
            r.exec("alsa_amixer cset numid=45 1");
            r.exec("alsa_amixer cset numid=39 3");
            r.exec("alsa_amixer cset numid=1 63");
            r.exec("alsa_amixer cset numid=6 7");
            r.exec("alsa_amixer cset numid=37 7");
            r.exec("alsa_amixer cset numid=13 5");
            r.exec("alsa_amixer cset numid=14 5");
            Process p = r.exec("uv2w_test -s \"AT+DMOGRP="+tx+","+rx+",0,0,4,0\"");
            r.exec("uv2w_test -s \"AT+DMOFUN=3,8,0,0,0\"");

            InputStream is = p.getInputStream();
            InputStreamReader isr = new InputStreamReader(is);
            BufferedReader br = new BufferedReader(isr);
            String line;

            while ((line = br.readLine()) != null) {
                Log.d(TAG, "======= "+line);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void runShellScript(String tx, String rx) {
        try {
            Log.d(TAG, "======== tx : "+tx+"  rx : "+rx);
            String[] cmd = { "sh", "-c", "/system/bin/wakie_talkie.sh "+tx+" "+rx};
            Process p = Runtime.getRuntime().exec(cmd);

            InputStream is = p.getInputStream();
            InputStreamReader isr = new InputStreamReader(is);
            BufferedReader br = new BufferedReader(isr);
            String line;

            while ((line = br.readLine()) != null) {
                Log.d(TAG, "======= "+line);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

    }
}
