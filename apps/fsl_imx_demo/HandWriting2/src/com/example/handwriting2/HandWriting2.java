/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Copyright (C) 2010-2012 Freescale Semiconductor, Inc. */

package com.example.handwriting2;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.view.MotionEvent;
import android.view.Menu;
import android.view.MenuItem;



public class HandWriting2 extends Activity {


    private static final int CLEAR_MENU_ID 	   = Menu.FIRST;

    private MyView    mUpPreview;
    private TextView  mDownPreview;


    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mUpPreview   = (MyView) findViewById(R.id.up_view);
        mDownPreview = (TextView) findViewById(R.id.down_view);
    }

    @Override
    public boolean onTrackballEvent(MotionEvent event) {
    	mUpPreview.onTrackballEvent(event);
        return true;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
    	mUpPreview.onTouchEvent(event);
        return true;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        menu.add(0, CLEAR_MENU_ID, 0, "Clear").setShortcut('3', 'c');
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case CLEAR_MENU_ID:
            	mUpPreview.clear();
                return true;

        }
        return super.onOptionsItemSelected(item);
    }
}
