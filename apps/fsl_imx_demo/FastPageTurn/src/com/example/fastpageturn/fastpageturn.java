/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2010-2012 Freescale Semiconductor, Inc.
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

package com.example.fastpageturn;

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
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.util.Log;


public class fastpageturn extends Activity {
	/** Called when the activity is first created. */

	MyView mView;

    	private final static String TAG = "fastpageturn";


	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mView = new MyView(this);
		setContentView(mView);
		mView.requestFocus();
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (mView.onKeyUp(keyCode, event) == false)
			return super.onKeyUp(keyCode, event);
		else
			return true;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (mView.onKeyDown(keyCode, event) == false)
			return super.onKeyDown(keyCode, event);
		else
			return true;
	}

        @Override
        public boolean onTouchEvent(MotionEvent event) {
		super.onTouchEvent(event);
	     	mView.onTouchEvent(event);
	        return true;
	}

	public class MyView extends View {

		private Bitmap mBitmap;
		private final Rect mRect = new Rect();
		private final Rect mlastRect = new Rect();
		private final Bitmap TurnPages[] = new Bitmap[20];
		boolean mUpdateFlag = false;
		boolean mStartFlag = false;

		boolean mFirstPointFlag = true;
		boolean mNewRegionFlag = true;

		boolean mBitmapLoaded = false;
		boolean mCurrPageDrawn = false;

		private int mTurnState = TURN_STOP;
		private TimerTask mTimerTask = null;
		private Timer mTimer = null;
        	private int nTimer = 243;

		private int mPageIndex = 0;

		private int mCurrPageIndex = 0;
		private int mNextPageIndex = 0;
		private boolean mNextPageReady = false;

		private int mCurrKeyCode = 0;
		private long mCurrKeyCodeDownTime = 0;
		private long mCurrTimeMillis = 0;

		/* touchsreen */
		private float mTouchDownX = 0;
		private float mTouchDownY = 0;
		private long  mTouchDownTime = 0;
		private float mTouchMoveX = 0;
		private float mTouchMoveY = 0;

		private float mTouchUpTime = 0;

		private long mMoveCount = 0;

		/** Used as a pulse to gradually fade the contents of the window. */
		private static final int UPDATE_MSG = 1;
		private static final int MONITOR_MSG = 2;
		/** How often to fade the contents of the window (in ms). */
		private int UPDATE_DELAY = 320; // 243;
		private boolean bAllowTimeToChange = false;

		/* update mode for handwriting in eink */
		private static final int UPDATE_MODE_WAIT = EINK_AUTO_MODE_REGIONAL | EINK_WAIT_MODE_WAIT | EINK_WAVEFORM_MODE_ANIM |EINK_UPDATE_MODE_FULL;
		private static final int UPDATE_MODE_FULL = EINK_AUTO_MODE_REGIONAL | EINK_WAIT_MODE_WAIT | EINK_WAVEFORM_MODE_GC16 |EINK_UPDATE_MODE_FULL;

		private static final int TURN_STOP = 0;
		private static final int TURN_FORWARD = 1;
		private static final int TURN_BACKWARD = 2;

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
			TurnPages[0] = loadBitmap(r.getDrawable(R.drawable.ebook01));
			TurnPages[1] = loadBitmap(r.getDrawable(R.drawable.ebook02));
			TurnPages[2] = loadBitmap(r.getDrawable(R.drawable.ebook03));
			TurnPages[3] = loadBitmap(r.getDrawable(R.drawable.ebook04));
			TurnPages[4] = loadBitmap(r.getDrawable(R.drawable.ebook05));
			TurnPages[5] = loadBitmap(r.getDrawable(R.drawable.ebook06));
			TurnPages[6] = loadBitmap(r.getDrawable(R.drawable.ebook07));

			TurnPages[7] = loadBitmap(r.getDrawable(R.drawable.ebook08));
			TurnPages[8] = loadBitmap(r.getDrawable(R.drawable.ebook09));
			TurnPages[9] = loadBitmap(r.getDrawable(R.drawable.ebook10));
			TurnPages[10] = loadBitmap(r.getDrawable(R.drawable.ebook11));
			TurnPages[11] = loadBitmap(r.getDrawable(R.drawable.ebook12));
			TurnPages[12] = loadBitmap(r.getDrawable(R.drawable.ebook13));
			TurnPages[13] = loadBitmap(r.getDrawable(R.drawable.ebook14));

			TurnPages[14] = loadBitmap(r.getDrawable(R.drawable.ebook15));
			TurnPages[15] = loadBitmap(r.getDrawable(R.drawable.ebook16));
			TurnPages[16] = loadBitmap(r.getDrawable(R.drawable.ebook17));
			TurnPages[17] = loadBitmap(r.getDrawable(R.drawable.ebook18));
			TurnPages[18] = loadBitmap(r.getDrawable(R.drawable.ebook19));

			TurnPages[19] = loadBitmap(r.getDrawable(R.drawable.ebook20));

			mBitmapLoaded = true;

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

		synchronized void drawNextPage(int update_mode) {

			if (!mNextPageReady) {
				if (mTurnState == TURN_FORWARD) {
					if (mCurrPageIndex < 19)
						mNextPageIndex = mCurrPageIndex + 1;
					else
						return;
				}
				else if (mTurnState == TURN_BACKWARD) {
					if (mCurrPageIndex > 0)
						mNextPageIndex = mCurrPageIndex - 1;
					else
						return;
				}
				else {
					mNextPageIndex = mCurrPageIndex;
				}

				mNextPageReady = true;
				// send a page
				invalidate(update_mode);
				Log.i(TAG, ("Invalidate a page : " + mNextPageIndex + "time at" + System.currentTimeMillis() + "\n"));
			}
			else {
				Log.i(TAG, ("Skip a draw request\n"));
			}
		}

		/**
		 * Start up the pulse to update the screen, clearing any existing pulse to
		 * ensure that we don't have multiple pulses running at a time.
		 */
		void startUpdating() {
		    mHandler.removeMessages(UPDATE_MSG);
		    mHandler.sendMessageDelayed(mHandler.obtainMessage(UPDATE_MSG), UPDATE_DELAY);
		}

		/**
		 * Stop the pulse to fade the screen.
		 */
		void stopUpdating() {
		    mHandler.removeMessages(UPDATE_MSG);
		}

		/**
		 * Start up the pulse to update the screen, clearing any existing pulse to
		 * ensure that we don't have multiple pulses running at a time.
		 */
		void startMonitor() {
		    mHandler.removeMessages(MONITOR_MSG);
		    mHandler.sendMessageDelayed(mHandler.obtainMessage(MONITOR_MSG), UPDATE_DELAY);
		}

		/**
		 * Stop the pulse to fade the screen.
		 */
		void stopMonitor() {
		    mHandler.removeMessages(MONITOR_MSG);
		}

		private Handler mHandler = new Handler() {
		    @Override public void handleMessage(Message msg) {
		        switch (msg.what) {
		            // Upon receiving the update pulse, we have the view perform a
		            // update and then enqueue a new message to pulse at the desired
		            // next time.
		            case UPDATE_MSG: {

		                if(mTurnState == TURN_STOP)
		                {
		                	stopUpdating();
		                }else
		                {
					drawNextPage(UPDATE_MODE_WAIT);
		                	mHandler.sendMessageDelayed(mHandler.obtainMessage(UPDATE_MSG), UPDATE_DELAY);
		                }
		                break;
		            }

		            case MONITOR_MSG: {

				mCurrTimeMillis = System.currentTimeMillis();

		                if(mTurnState == TURN_STOP)
		                {
					if ((mCurrTimeMillis - mTouchDownTime) >= 750) {
					    /* Long touch    */
					    if (mTouchDownY > mTouchMoveY) {
						/* fast forward */
						if (mTurnState != TURN_FORWARD) {
						    mTurnState = TURN_FORWARD;
						    Log.i(TAG, ("Touch_F long down\n"));
						    startUpdating();
						}
					    }
					    else {
						/* fast rewind */
						if (mTurnState != TURN_BACKWARD) {
						    Log.i(TAG, ("Touch_R long down\n"));
						    mTurnState = TURN_BACKWARD;
						    startUpdating();
						}
					    }
					}
					else {
				        	mHandler.sendMessageDelayed(mHandler.obtainMessage(MONITOR_MSG), UPDATE_DELAY);
					}
				}

		                break;
		            }

		            default:
		                super.handleMessage(msg);
		        }
		    }
		};

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

			Bitmap newBitmap = Bitmap.createBitmap(800, 600,
					Bitmap.Config.RGB_565);
			Canvas newCanvas = new Canvas();
			newCanvas.setBitmap(newBitmap);
			if (mBitmap != null) {
				newCanvas.drawBitmap(mBitmap, 0, 0, null);
			}
			mBitmap = newBitmap;

			Log.i(TAG, ("OnSizeChanged\n"));
			mNextPageReady = true;
			invalidate(UPDATE_MODE_FULL);
		}

		@Override
		protected void onDraw(Canvas canvas) {
			Log.i(TAG, ("Draw time at " + System.currentTimeMillis() + "\n"));
			Log.i(TAG, ("Draw time interval(ms) = " + (System.currentTimeMillis() - mCurrTimeMillis) + "\n"));
			mCurrTimeMillis = System.currentTimeMillis();

			if (mNextPageReady) {
				Log.i(TAG, ("onDraw a page = " + mNextPageIndex + "\n"));
				mCurrPageIndex = mNextPageIndex;
				mNextPageReady = false;

				if (mBitmapLoaded) {
					canvas.drawColor(Color.WHITE);
					canvas.drawBitmap(TurnPages[mCurrPageIndex], 0, 0, null);
					mCurrPageDrawn = true;
				}
			}
			else {
				Log.i(TAG, ("Cancel a draw or re-draw\n"));
				if (mBitmapLoaded) {
					canvas.drawColor(Color.WHITE);
					canvas.drawBitmap(TurnPages[mCurrPageIndex], 0, 0, null);
				}
			}
		}

		public boolean onKeyDown(int keyCode, KeyEvent event) {
			boolean ret=false;

			if (mCurrKeyCode != keyCode) {
				mCurrKeyCode = keyCode;
				mCurrKeyCodeDownTime = event.getDownTime();
			}
			else {
				if ((event.getDownTime() - mCurrKeyCodeDownTime) >= 1000) {
					switch (keyCode) {
					case KeyEvent.KEYCODE_F: // Start FF
						if (mTurnState != TURN_FORWARD) {
							mTurnState = TURN_FORWARD;
							Log.i(TAG, ("KEYCODE_F long down\n"));
							startUpdating();
							ret = true;
						}
						break;
					case KeyEvent.KEYCODE_R: // Start REW
						if (mTurnState != TURN_BACKWARD) {
							Log.i(TAG, ("KEYCODE_R long down\n"));
							mTurnState = TURN_BACKWARD;
							startUpdating();
							ret = true;
						}
						break;
					}

				}
			}
			return ret;
		}

		public boolean onKeyUp(int keyCode, KeyEvent event) {
			boolean ret=false;

			/* mTurnState will be checked to decide the action next
				if mTurnState is TURN_STOP,
					then just turn one page F or R with Full update mode
				if mTurnState is TURN_FORWARD/TURN_BACKWARD,
					then stop turn and re-draw current page with Full update mode
                        */

			mCurrKeyCode = 0;
			mCurrKeyCodeDownTime = 0;
			mNextPageReady = false;
			mNextPageIndex = mCurrPageIndex;

			Log.i(TAG, ("Key up: mTurnState = " + mTurnState + "\n"));
			if (mTurnState == TURN_STOP) {
				Log.i(TAG, ("Key click released\n"));
				switch (keyCode) {
				case KeyEvent.KEYCODE_F: // Next
					// draw next page in Full mode
					mTurnState = TURN_FORWARD;
					drawNextPage(UPDATE_MODE_FULL);
					mTurnState = TURN_STOP;
					ret = true;
					break;
				case KeyEvent.KEYCODE_R: // Last
					// draw previous page in Full mode
					mTurnState = TURN_BACKWARD;
					drawNextPage(UPDATE_MODE_FULL);
					mTurnState = TURN_STOP;
					ret = true;
					break;
				}
			}
			else {
				Log.i(TAG, ("Key long released (Cancel) \n"));
				switch (keyCode) {
				case KeyEvent.KEYCODE_F: // Next
				case KeyEvent.KEYCODE_R: // Last
					// draw current page in Full mode
					mTurnState = TURN_STOP;
					drawNextPage(UPDATE_MODE_FULL);
					ret = true;
					break;
				}
			}
			return ret;
		}

		@Override
		public boolean onTouchEvent(MotionEvent event) {
			Log.i(TAG, ("Touch event = " + event.getAction() + " !! \n"));
		    switch (event.getAction()) {

			case MotionEvent.ACTION_MOVE:
				mTouchMoveX = event.getX();
				mTouchMoveY = event.getY();
				break;
		        case MotionEvent.ACTION_DOWN:

				Log.i(TAG, ("Touch Down !! \n"));
				mTouchDownX = event.getX();
				mTouchDownY = event.getY();
				mTouchDownTime = System.currentTimeMillis(); // event.getDownTime();
				startMonitor();
		            break;
		        case MotionEvent.ACTION_UP:

				stopMonitor();
				stopUpdating();

				mNextPageReady = false;
				mNextPageIndex = mCurrPageIndex;

				mTouchUpTime = System.currentTimeMillis();

				Log.i(TAG, ("Touch up: mTurnState = " + mTurnState + "time(s) = " + mTouchUpTime + "time(ev)" + event.getEventTime() + "\n"));
//				if ((mTurnState == TURN_STOP) || ((event.getEventTime() - mTouchDownTime) < 1000)) {
				if (mTurnState == TURN_STOP) {
					Log.i(TAG, ("Touch click released" + "getY()=" + event.getY() + " cur=" + mTouchDownY +"\n"));
					if (event.getY() < mTouchDownY){
						Log.i(TAG, ("next page\n"));
						// draw next page in Full mode
						mTurnState = TURN_FORWARD;
						drawNextPage(UPDATE_MODE_FULL);
						mTurnState = TURN_STOP;
					}
					else {
						Log.i(TAG, ("previous page\n"));
						// draw previous page in Full mode
						mTurnState = TURN_BACKWARD;
						drawNextPage(UPDATE_MODE_FULL);
						mTurnState = TURN_STOP;
					}
				}
				else {
					Log.i(TAG, ("Touch long released (Cancel) \n"));
					// draw current page in Full mode
					mTurnState = TURN_STOP;
//					try {
//						Thread.sleep(300);
//					}
//					catch(InterruptedException e) {}
					drawNextPage(UPDATE_MODE_FULL);
				}

				mTouchDownX = 0;
				mTouchDownY = 0;
				mTouchDownTime = 0;
				mMoveCount = 0;

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
		    if (TurnPages[15] != null) {
		        TurnPages[15].recycle();
		    }
		    if (TurnPages[16] != null) {
		        TurnPages[16].recycle();
		    }
		    if (TurnPages[17] != null) {
		        TurnPages[17].recycle();
		    }
		    if (TurnPages[18] != null) {
		        TurnPages[18].recycle();
		    }
		    if (TurnPages[19] != null) {
		        TurnPages[19].recycle();
		    }

		}

	}
}
