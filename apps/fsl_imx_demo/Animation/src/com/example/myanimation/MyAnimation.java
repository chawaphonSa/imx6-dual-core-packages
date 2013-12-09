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

package com.example.myanimation;

import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.BitmapFactory.Options;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.util.Log;

public class MyAnimation extends Activity {
	/** Called when the activity is first created. */

	MyView mView;

    private final static String TAG = "Animation";


	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mView = new MyView(this);
		setContentView(mView);
		mView.requestFocus();
	}
    @Override
    public boolean onTouchEvent(MotionEvent event) {
    	mView.onTouchEvent(event);
        return true;
    }


	public class MyView extends View {

		private Bitmap mBitmap;
		private final Rect mRect = new Rect();
		private final Rect mlastRect = new Rect();
		private final Bitmap TurnPages[] = new Bitmap[15];
		boolean mUpdateFlag = false;
		boolean mStartFlag = false;

		boolean mFirstPointFlag = true;
		boolean mNewRegionFlag = true;

		boolean mBitmapLoaded = false;

		private int mTurn = TURN_FORWARD;
		private TimerTask mTimerTask = null;
		private Timer mTimer = null;

		private int mPageIndex = 0;
        private int mInterval = 160;
		private int mCycle = 0;

		/* update mode for handwriting in eink */
		private static final int UPDATE_MODE_PARTIAL = EINK_AUTO_MODE_REGIONAL| EINK_WAIT_MODE_NOWAIT | EINK_WAVEFORM_MODE_ANIM | EINK_UPDATE_MODE_PARTIAL;;
		private static final int UPDATE_MODE_FULL =EINK_AUTO_MODE_REGIONAL| EINK_WAIT_MODE_WAIT | EINK_WAVEFORM_MODE_AUTO | EINK_UPDATE_MODE_FULL;

		private static final int TURN_STOP = 0;
		private static final int TURN_FORWARD = 1;

		void changeText() {
			if (mTurn == TURN_STOP)
				return;

			if (mTurn == TURN_FORWARD) {
                mCycle++;

                if(mCycle == 72)
                {
      			    postInvalidate(UPDATE_MODE_FULL);
                    mCycle=0;

                }
                else
                {
      				postInvalidate(UPDATE_MODE_PARTIAL);
				    if (mPageIndex < 14)
                    {
					    mPageIndex++;
                    }
				    else
                    {
					    mPageIndex = 0;
                    }
                }
			}
		}

		public MyView(Context c) {
			super(c);

			mRect.left = 0;
			mRect.top = 0;
			mRect.right = 0;
			mRect.bottom = 0;

			mlastRect.left = 0;
			mlastRect.top = 0;
			mlastRect.right = 0;
			mlastRect.bottom = 0;

			mFirstPointFlag = true;

			Resources r = getResources();
			TurnPages[0] = loadBitmap(r.getDrawable(R.drawable.a1));
			TurnPages[1] = loadBitmap(r.getDrawable(R.drawable.a2));
			TurnPages[2] = loadBitmap(r.getDrawable(R.drawable.a3));
			TurnPages[3] = loadBitmap(r.getDrawable(R.drawable.a4));
			TurnPages[4] = loadBitmap(r.getDrawable(R.drawable.a5));
			TurnPages[5] = loadBitmap(r.getDrawable(R.drawable.a6));
			TurnPages[6] = loadBitmap(r.getDrawable(R.drawable.a7));
			TurnPages[7] = loadBitmap(r.getDrawable(R.drawable.a8));
			TurnPages[8] = loadBitmap(r.getDrawable(R.drawable.a9));
			TurnPages[9] = loadBitmap(r.getDrawable(R.drawable.a10));
			TurnPages[10] = loadBitmap(r.getDrawable(R.drawable.a11));
			TurnPages[11] = loadBitmap(r.getDrawable(R.drawable.a12));
			TurnPages[12] = loadBitmap(r.getDrawable(R.drawable.a13));
			TurnPages[13] = loadBitmap(r.getDrawable(R.drawable.a14));
			TurnPages[14] = loadBitmap(r.getDrawable(R.drawable.a15));

			mBitmapLoaded = true;

			mTimer = new Timer();
			mTimerTask = new TimerTask() {
				public void run() {
					changeText();
				}
			};
			mTimer.schedule(mTimerTask, 0, mInterval);

		}

		public Bitmap loadBitmap(Drawable sprite, Bitmap.Config bitmapConfig) {
			int width = sprite.getIntrinsicWidth();
			int height = sprite.getIntrinsicHeight();
			Bitmap bitmap = Bitmap.createBitmap(width, height, bitmapConfig);
			Canvas canvas = new Canvas(bitmap);
			sprite.setBounds(0, 0, width, height);
			sprite.draw(canvas);
			return bitmap;
		}

		public Bitmap loadBitmap(Drawable sprite) {
			return loadBitmap(sprite, Bitmap.Config.RGB_565);
		}

		@Override
		protected void onSizeChanged(int w, int h, int oldw, int oldh) {
			int curW = mBitmap != null ? mBitmap.getWidth() : 0;
			int curH = mBitmap != null ? mBitmap.getHeight() : 0;
			if (curW >= w && curH >= h) {
				return;
			}

			if (curW < w)
				curW = w;
			if (curH < h)
				curH = h;

			Bitmap newBitmap = Bitmap.createBitmap(480, 480,
					Bitmap.Config.RGB_565);
			Canvas newCanvas = new Canvas();
			newCanvas.setBitmap(newBitmap);
			if (mBitmap != null) {
				newCanvas.drawBitmap(mBitmap, 0, 0, null);
			}
			mBitmap = newBitmap;
		}

		@Override
		protected void onDraw(Canvas canvas) {
			if (mBitmapLoaded) {
				canvas.drawColor(Color.WHITE);
                if(mCycle > 0)
				    canvas.drawBitmap(TurnPages[mPageIndex], 0, 0, null);
			}
		}

        @Override
        public boolean onTouchEvent(MotionEvent event) {

            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    mTimer.cancel();
                    mTimer.purge();
                    break;
                case MotionEvent.ACTION_MOVE:
                    break;
                case MotionEvent.ACTION_UP:
                    if( event.getX() < 400 )
                    {
                        mInterval--;
                        Log.e(TAG, "increase the timer = " + mInterval);
                    }
                    else
                    {
                        mInterval++;
                        Log.e(TAG, "reduce the timer" + mInterval);
                    }
                    try {
                    mTimer = new Timer();
                    mTimerTask = new TimerTask() {
                        public void run() {
                            changeText();
                        }
                    };
                    mTimer.schedule(mTimerTask, 0, mInterval);
                    } catch( Exception e) {
                        Log.e(TAG, "Timer schedule failed", e );
                    }
                    break;
            }
            return true;
        }

        protected void onDetachedFromWindow() {
            super.onDetachedFromWindow();
            if (TurnPages[0] != null) {
                TurnPages[0].recycle();
            }
            if (TurnPages[1] != null) {
                TurnPages[1].recycle();
            }
            if (TurnPages[2] != null) {
                TurnPages[2].recycle();
            }
            if (TurnPages[3] != null) {
                TurnPages[3].recycle();
            }
            if (TurnPages[4] != null) {
                TurnPages[4].recycle();
            }
            if (TurnPages[5] != null) {
                TurnPages[5].recycle();
            }
            if (TurnPages[6] != null) {
                TurnPages[6].recycle();
            }
            if (TurnPages[7] != null) {
                TurnPages[7].recycle();
            }
            if (TurnPages[8] != null) {
                TurnPages[8].recycle();
            }
            if (TurnPages[9] != null) {
                TurnPages[9].recycle();
            }
            if (TurnPages[10] != null) {
                TurnPages[10].recycle();
            }
            if (TurnPages[11] != null) {
                TurnPages[11].recycle();
            }
            if (TurnPages[12] != null) {
                TurnPages[12].recycle();
            }
            if (TurnPages[13] != null) {
                TurnPages[13].recycle();
            }
            if (TurnPages[14] != null) {
                TurnPages[14].recycle();
            }
        }

    }
}



