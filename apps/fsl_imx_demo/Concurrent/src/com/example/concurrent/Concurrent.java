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

package com.example.concurrent;

import java.util.Random;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Color;
import android.os.Bundle;
import android.view.KeyEvent;

public class Concurrent extends Activity {

    SampleView myView;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
        myView = new SampleView(this);
		setContentView(myView);
	}

    @Override
    protected void onResume() {
        super.onResume();
        myView.onResume();
    }

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (myView.onKeyUp(keyCode, event) == false)
			return super.onKeyUp(keyCode, event);
		else
			return true;
	}

	private class SampleView extends TileView {
        private static final int UPDATE_MODE_FULL = EINK_AUTO_MODE_REGIONAL |
        EINK_WAIT_MODE_WAIT | EINK_WAVEFORM_MODE_GC16 | EINK_UPDATE_MODE_FULL;
	    private static final int mMoveDelay = 150;
	    private static final int mNumConcurrent = 1;
	    private static final int mTotalUniqueBitmap = 10;
	    private final Random RNG = new Random();

	    private int[][] mChangeGrid;
	    private int	mChangeCount;
        public int mDelay = 10;

	    void update() {
	    	int x, y ,bitmap;
    		for( int i = 0; i < mNumConcurrent; i++ )
    		{
    			mChangeCount++;
    			do
    			{
	    			x = RNG.nextInt(mXTileCount);
	    			y = RNG.nextInt(mYTileCount);
    			}
    			while( (mChangeCount - mChangeGrid[x][y]) < (int)(mXTileCount*mYTileCount*0.9) );
    			mChangeGrid[x][y] = mChangeCount;
    			bitmap =  ( getTile(x, y) + RNG.nextInt(mTotalUniqueBitmap - 1) ) % mTotalUniqueBitmap;

    			setTile( bitmap, x, y);
                if( mDelay > 0 )
                {
                    postInvalidate(mMoveDelay * mDelay);
                    mDelay--;
                }
                else
                    postInvalidate(mMoveDelay, x, y);
    		}
        }

	    public void onResume() {
            mDelay = 10;
            postInvalidate(UPDATE_MODE_FULL);
        }

		private void initView() {

	        Resources r = this.getContext().getResources();

	        resetTiles(mTotalUniqueBitmap);
	        loadTile(0, r.getDrawable(R.drawable.p0));
	        loadTile(1, r.getDrawable(R.drawable.p1));
	        loadTile(2, r.getDrawable(R.drawable.p2));
	        loadTile(3, r.getDrawable(R.drawable.p3));
	        loadTile(4, r.getDrawable(R.drawable.p4));
	        loadTile(5, r.getDrawable(R.drawable.p5));
	        loadTile(6, r.getDrawable(R.drawable.p6));
	        loadTile(7, r.getDrawable(R.drawable.p7));
	        loadTile(8, r.getDrawable(R.drawable.p8));
	        loadTile(9, r.getDrawable(R.drawable.p9));

	        for( int i = 0; i < mXTileCount; i++ )
	        	for( int j = 0; j < mYTileCount; j++ )
                {
	        		setTile(RNG.nextInt(mTotalUniqueBitmap), i, j);
	        		mChangeGrid[i][j] = j*mXTileCount + i;
                }
            invalidate(UPDATE_MODE_FULL);
	    }

		public SampleView(Context context) {
			super(context);
            invalidate(UPDATE_MODE_FULL);
		}

	    @Override
	    public void onDraw(Canvas canvas) {
            if( mDelay > 0 )
                canvas.drawColor(Color.WHITE);
            else
                super.onDraw(canvas);
	    	update();
	    }

	    @Override
	    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		    super.onSizeChanged(w, h, oldw, oldh);
	        mChangeGrid = new int[mXTileCount][mYTileCount];
	        mChangeCount = mXTileCount*mYTileCount;
			initView();
	    }

		public boolean onKeyUp(int keyCode, KeyEvent event) {
            boolean ret=false;

			switch (keyCode) {
			case KeyEvent.KEYCODE_S: // Stop
				mDelay = 10;
				ret = true;
				break;
			}

			return ret;
		}

	}
}
