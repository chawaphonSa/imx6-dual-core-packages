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

import android.content.Context;

import android.graphics.Bitmap;

import android.graphics.Canvas;

import android.graphics.Paint;

import android.graphics.Color;

import android.graphics.drawable.Drawable;

import android.view.View;

/**
 *
 * TileView: a View-variant designed for handling arrays of "icons" or other
 *
 * drawables.
 *
 *
 */

public class TileView extends View {

	/**
	 *
	 * Parameters controlling the size of the tiles and their range within view.
	 *
	 * Width/Height are in pixels, and Drawables will be scaled to fit to these
	 *
	 * dimensions. X/Y Tile Counts are the number of tiles that will be drawn.
	 */

	 private static final int UPDATE_MODE_PARTIAL = EINK_AUTO_MODE_REGIONAL |
	 EINK_WAIT_MODE_NOWAIT | EINK_WAVEFORM_MODE_GC16 | EINK_UPDATE_MODE_PARTIAL;

	 private static final int UPDATE_MODE_FULL = EINK_AUTO_MODE_REGIONAL |
	 EINK_WAIT_MODE_NOWAIT | EINK_WAVEFORM_MODE_GC16 | EINK_UPDATE_MODE_FULL;

	protected static int mTileSizeX = 160;

    protected static int mTileSizeY = (int)(mTileSizeX/800.0*600);

	protected static int mXTileCount = 4;

	protected static int mYTileCount = 4;

	private static int mXOffset;

	private static int mYOffset;

	/**
	 *
	 * A hash that maps integer handles specified by the subclasser to the
	 *
	 * drawable that will be used for that reference
	 */

	private Bitmap[] mTileArray = null;

	/**
	 *
	 * A two-dimensional array of integers in which the number represents the
	 *
	 * index of the tile that should be drawn at that locations
	 */

	private int[][] mTileGrid;

	private final Paint mPaint = new Paint();

	public TileView(Context context) {

		super(context);

	}

	/**
	 *
	 * Rests the internal array of Bitmaps used for drawing tiles, and
	 *
	 * sets the maximum index of tiles to be inserted
	 *
	 *
	 *
	 * @param tilecount
	 */

	public void resetTiles(int tilecount) {

		mTileArray = new Bitmap[tilecount];

	}

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {

		mXOffset = ((w - (mTileSizeX * mXTileCount)) / (mXTileCount+1));

		mYOffset = ((h - (mTileSizeY * mYTileCount)) / (mYTileCount+1));

		mTileGrid = new int[mXTileCount][mYTileCount];

		clearTiles();

	}

	/**
	 *
	 * Function to set the specified Drawable as the tile for a particular
	 *
	 * integer key.
	 *
	 *
	 *
	 * @param key
	 *
	 * @param tile
	 */

	public void loadTile(int key, Drawable tile) {

		Bitmap bitmap = Bitmap.createBitmap(mTileSizeX, mTileSizeY,
				Bitmap.Config.RGB_565);

		Canvas canvas = new Canvas(bitmap);

		tile.setBounds(0, 0, mTileSizeX, mTileSizeY);

		tile.draw(canvas);

		mTileArray[key] = bitmap;

	}

	/**
	 *
	 * Resets all tiles to 0 (empty)
	 *
	 *
	 */

	public void clearTiles() {

		for (int x = 0; x < mXTileCount; x++) {

			for (int y = 0; y < mYTileCount; y++) {

				setTile(0, x, y);

			}

		}

	}

	public void invalidate(int x, int y) {

		// TODO Auto-generated method stub

		invalidate( mXOffset + x * mXOffset + x * mTileSizeX, mYOffset + y * mYOffset + y * mTileSizeY,

		mXOffset + x * mXOffset + (x+1) * mTileSizeX, mYOffset + y * mYOffset + (y+1) * mTileSizeY, UPDATE_MODE_FULL);

	}

	public void postInvalidate(int delay, int x, int y) {

		// TODO Auto-generated method stub

		postInvalidateDelayed(delay, mXOffset + x * mXOffset + x * mTileSizeX, mYOffset + y * mYOffset + y * mTileSizeY,

		mXOffset + x * mXOffset + (x+1) * mTileSizeX, mYOffset + y * mYOffset + (y+1) * mTileSizeY, UPDATE_MODE_FULL);

	}

	/**
	 *
	 * Used to indicate that a particular tile (set with loadTile and referenced
	 *
	 * by an integer) should be drawn at the given x/y coordinates during the
	 *
	 * next invalidate/draw cycle.
	 *
	 *
	 *
	 * @param tileindex
	 *
	 * @param x
	 *
	 * @param y
	 */

	public void setTile(int tileindex, int x, int y) {

		if (mTileArray != null)

			if (tileindex >= mTileArray.length)

				tileindex = mTileArray.length - 1;

		mTileGrid[x][y] = tileindex;

	}

	public int getTile(int x, int y) {

		return mTileGrid[x][y];

	}

	@Override
	public void onDraw(Canvas canvas) {

		super.onDraw(canvas);

		canvas.drawColor(Color.WHITE);

        if( mTileArray != null )
        {
            for (int x = 0; x < mXTileCount; x += 1) {
                for (int y = 0; y < mYTileCount; y += 1) {
                    canvas.drawBitmap(mTileArray[mTileGrid[x][y]],
                    mXOffset + x * (mXOffset + mTileSizeX),
                    mYOffset + y * (mYOffset + mTileSizeY),
                    mPaint);
                }
            }
        }
	}
}
