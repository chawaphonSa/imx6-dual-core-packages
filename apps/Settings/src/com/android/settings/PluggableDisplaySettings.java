/*
 * Copyright (C) 2010 The Android Open Source Project
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

/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 */

package com.android.settings;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ActivityManagerNative;
import android.app.admin.DevicePolicyManager;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.DialogInterface;
import android.database.ContentObserver;
import android.os.Message;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.DisplayManager;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.SeekBarPreference;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.preference.PreferenceCategory;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.Log;
import android.view.IWindowManager;
import android.view.Surface;
import android.view.View;

import java.util.ArrayList;

public class PluggableDisplaySettings extends SettingsPreferenceFragment implements
        Preference.OnPreferenceChangeListener,DialogInterface.OnClickListener {

    private static final String TAG = "PluggableDisplaySettings";
    private static final boolean DBG = true;

    private static final int MAX_DISPLAY_DEVICE = 6;    

    private static final String[] KEY_DISPLAY_ENABLE     = {"display_enable_0","display_enable_1","display_enable_2",
                                                            "display_enable_3","display_enable_4","display_enable_5"};
    private static final String[] KEY_DISPLAY_MIRROR     = {"display_mirror_0","display_mirror_1","display_mirror_2",
                                                            "display_mirror_3","display_mirror_4","display_mirror_5"};
    private static final String[] KEY_DISPLAY_XOVERSCAN   = {"display_xoverscan_0","display_xoverscan_1","display_xoverscan_2",
                                                            "display_xoverscan_3","display_xoverscan_4","display_xoverscan_5"};
    private static final String[] KEY_DISPLAY_YOVERSCAN   = {"display_yoverscan_0","display_yoverscan_1","display_yoverscan_2",
                                                            "display_yoverscan_3","display_yoverscan_4","display_yoverscan_5"};
    private static final String[] KEY_DISPLAY_MODE       = {"display_mode_0","display_mode_1","display_mode_2",
                                                            "display_mode_3","display_mode_4","display_mode_5"};
    private static final String[] KEY_DISPLAY_ROTATION   = {"display_rotation_0","display_rotation_1","display_rotation_2",
                                                            "display_rotation_3","display_rotation_4","display_rotation_5"};
    private static final String[] KEY_DISPLAY_COLORDEPTH = {"display_colordepth_0","display_colordepth_1","display_colordepth_2",
                                                            "display_colordepth_3","display_colordepth_4","display_colordepth_5"};
    private static final String[] KEY_DISPLAY_KEEPRATE   = {"display_keeprate_0","display_keeprate_1","display_keeprate_2",
                                                            "display_keeprate_3","display_keeprate_4","display_keeprate_5"};
    private static final String[] KEY_DISPLAY_CATEGORY   = {"display_category_0","display_category_1","display_category_2",
                                                            "display_category_3","display_category_4","display_category_5"};
    private String[] mHighMode = new String[MAX_DISPLAY_DEVICE];
    private String[] mCurrentMode = new String[MAX_DISPLAY_DEVICE];
    private static final String DISPLAY_HIGH_MODE = "keepHighestMode";
    private static final String DISPLAY_HIGH_MODE_ENTRY = "Keep Highest Mode";

    private DisplayManager mDisplayManager;
    
    private final Configuration mCurConfig = new Configuration();

    private CheckBoxPreference[] mDisplayEnablePref = new CheckBoxPreference[MAX_DISPLAY_DEVICE];
    private CheckBoxPreference[] mMirrorPref        = new CheckBoxPreference[MAX_DISPLAY_DEVICE];
    private CheckBoxPreference[] mKeepHModePref        = new CheckBoxPreference[MAX_DISPLAY_DEVICE];
    private CheckBoxPreference[] mRotationPref      = new CheckBoxPreference[MAX_DISPLAY_DEVICE];
    private SeekBarPreference[]  mXOverScanPref      = new SeekBarPreference[MAX_DISPLAY_DEVICE];
    private SeekBarPreference[]  mYOverScanPref      = new SeekBarPreference[MAX_DISPLAY_DEVICE];
    private ListPreference[]     mDisplayModePref   = new ListPreference[MAX_DISPLAY_DEVICE];
    private ListPreference[]     mColorDepthPref    = new ListPreference[MAX_DISPLAY_DEVICE];
    private ListPreference[]     mKeepRatePref    = new ListPreference[MAX_DISPLAY_DEVICE];
    private PreferenceCategory[] mCategoryPref      = new PreferenceCategory[MAX_DISPLAY_DEVICE];
    
    private IntentFilter mIntentFilter;
    
    private Dialog mConfirmDialog;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if((DisplayManager.ACTION_DISPLAY_DEVICE_0_ATTACHED.equals(action)) ||
               (DisplayManager.ACTION_DISPLAY_DEVICE_1_ATTACHED.equals(action)) ||
               (DisplayManager.ACTION_DISPLAY_DEVICE_2_ATTACHED.equals(action)) ||
               (DisplayManager.ACTION_DISPLAY_DEVICE_3_ATTACHED.equals(action)) ||
               (DisplayManager.ACTION_DISPLAY_DEVICE_4_ATTACHED.equals(action)) ||
               (DisplayManager.ACTION_DISPLAY_DEVICE_5_ATTACHED.equals(action))) {
                int dispid = intent.getIntExtra(DisplayManager.EXTRA_DISPLAY_DEVICE, -1);
                boolean connect = intent.getBooleanExtra(DisplayManager.EXTRA_DISPLAY_CONNECT, false);
                handleDisplayConnected(dispid, connect);
            } else if (DisplayManager.ACTION_DISPLAY_STATE.equals(action)) {
                int dispid = intent.getIntExtra(DisplayManager.EXTRA_DISPLAY_DEVICE, -1);
                handleDisplayStateChanged(dispid, intent.getIntExtra(
                DisplayManager.EXTRA_DISPLAY_STATE, DisplayManager.DISPLAY_STATE_UNKNOWN));
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ContentResolver resolver = getActivity().getContentResolver();

        addPreferencesFromResource(R.xml.pluggable_display_settings);

        mDisplayManager = (DisplayManager)getSystemService(Context.DISPLAYMANAGER_SERVICE);

        for(int i=0; i<MAX_DISPLAY_DEVICE; i++){
            mDisplayModePref[i] = null;
            mDisplayEnablePref[i] = null;
            mMirrorPref[i] = null;
            mKeepHModePref[i] = null;
            mXOverScanPref[i] = null;
            mYOverScanPref[i] = null;
            mRotationPref[i] = null;
            mColorDepthPref[i] = null;
            mKeepRatePref[i] = null;
            mCategoryPref[i] = null;
        }

        mIntentFilter = new IntentFilter(DisplayManager.ACTION_DISPLAY_DEVICE_1_ATTACHED);
        mIntentFilter.addAction(DisplayManager.ACTION_DISPLAY_DEVICE_2_ATTACHED);
        mIntentFilter.addAction(DisplayManager.ACTION_DISPLAY_DEVICE_3_ATTACHED);
        mIntentFilter.addAction(DisplayManager.ACTION_DISPLAY_DEVICE_4_ATTACHED);        
        mIntentFilter.addAction(DisplayManager.ACTION_DISPLAY_STATE);
    }

    @Override
    public void onResume() {
        super.onResume();
        updateState();
        getActivity().registerReceiver(mReceiver, mIntentFilter);
    }

    @Override
    public void onPause() {
        super.onPause();

        getActivity().unregisterReceiver(mReceiver);
    }

    private void updateState() {

    }

    private void updateDisplayModePreferenceDescription(int dispid, String CurrentDisplayMode) {
        ListPreference preference = mDisplayModePref[dispid];
        if(DISPLAY_HIGH_MODE.equals(CurrentDisplayMode) && !mHighMode[dispid].isEmpty()) {
            CurrentDisplayMode = mHighMode[dispid];
        }
        preference.setSummary(CurrentDisplayMode);
    }

    private void updateDisplayColorDepthPreferenceDescription(int dispid,int CurrentDisplayColorDepth) {
        ListPreference preference = mColorDepthPref[dispid];
        mColorDepthPref[dispid].setValue(String.valueOf(CurrentDisplayColorDepth));
        preference.setSummary(mColorDepthPref[dispid].getEntry());
    }
    
    private void updateActionModePreferenceDescription(int dispid, CharSequence CurrentActionMode) {
        ListPreference preference = mKeepRatePref[dispid];
        preference.setSummary(CurrentActionMode);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        if(DBG) Log.w(TAG, "onPreferenceTreeClick ");

        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    public boolean onPreferenceChange(Preference preference, Object objValue) {
        if(DBG) Log.w(TAG, "onPreferenceChange ");
        final String key = preference.getKey();
        int dispid = 0;

        for(int i=0; i<MAX_DISPLAY_DEVICE; i++ ) {
            dispid = i;
            if (KEY_DISPLAY_MODE[i].equals(key)) {
                String value = (String) objValue;
                //special case for mode change
                //it should reset keeprate overscan.
                boolean modeChanged = true;
                String lastMode = mCurrentMode[i];
                mCurrentMode[i] = value;
                if(lastMode.isEmpty()) {
                    lastMode = value;
                }
                if(mHighMode[i].isEmpty()) {
                    mHighMode[i] = value;
                }

                if(lastMode.equals(DISPLAY_HIGH_MODE) && mCurrentMode[i].equals(mHighMode[i])) {
                    modeChanged = false;
                }
                if(lastMode.equals(mHighMode[i]) && mCurrentMode[i].equals(DISPLAY_HIGH_MODE)) {
                    modeChanged = false;
                }
                if(lastMode.equals(mCurrentMode[i])) {
                    modeChanged = false;
                }

                if(modeChanged) {
                    mDisplayManager.setDisplayMode(dispid, value);
                    if(mKeepRatePref[dispid] != null) {
                        int keepRate = mDisplayManager.getDisplayKeepRate(dispid);
                        mKeepRatePref[dispid].setValue(Integer.toHexString(keepRate));
                        updateActionModePreferenceDescription(dispid, mKeepRatePref[dispid].getEntry());
                    }

                    if(mXOverScanPref[dispid] != null) {
                        mXOverScanPref[dispid].setProgress(0);
                    }
                    if(mYOverScanPref[dispid] != null) {
                        mYOverScanPref[dispid].setProgress(0);
                    }
                }
                updateDisplayModePreferenceDescription(dispid, value);

                break;
            }
        
            if (KEY_DISPLAY_ENABLE[i].equals(key)) {
                boolean value = (Boolean) objValue;
                mDisplayManager.setDisplayEnable(dispid, value);
                break;
            }
        
            if (KEY_DISPLAY_MIRROR[i].equals(key)) {
                boolean value = (Boolean) objValue;
                mDisplayManager.setDisplayMirror(dispid, value);      
                break;
            }
            
            if (KEY_DISPLAY_XOVERSCAN[i].equals(key)) {
                int value = Integer.parseInt(objValue.toString());            
                    mDisplayManager.setDisplayXOverScan(dispid, value);
                    View rootView = getActivity().getWindow().peekDecorView();
                    if(rootView != null) {
                        rootView.postInvalidateDelayed(200);
                    }

                break;
            }

            if (KEY_DISPLAY_YOVERSCAN[i].equals(key)) {
                int value = Integer.parseInt(objValue.toString());            
                    mDisplayManager.setDisplayYOverScan(dispid, value);
                    View rootView = getActivity().getWindow().peekDecorView();
                    if(rootView != null) {
                        rootView.postInvalidateDelayed(200);
                    }

                break;
            }

            if (KEY_DISPLAY_ROTATION[i].equals(key)) {
                boolean value = (Boolean) objValue;
                mDisplayManager.setDisplayRotation(dispid, value);        
                break;
            }
        
            if (KEY_DISPLAY_COLORDEPTH[i].equals(key)) {
                int value = Integer.parseInt((String) objValue);
                mDisplayManager.setDisplayColorDepth(dispid, value);         
                updateDisplayColorDepthPreferenceDescription(dispid, value);
                View rootView = getActivity().getWindow().peekDecorView();
                if(rootView != null) {
                    rootView.postInvalidateDelayed(500);
                }

                break;
            }
        
            if (KEY_DISPLAY_KEEPRATE[i].equals(key)) {
                int value = Integer.parseInt((String) objValue, 16);
                Log.w(TAG, "onPreferenceChange keeprate value=" + value);;
                mDisplayManager.setDisplayKeepRate(dispid, value);         
                mKeepRatePref[dispid].setValue((String)objValue);
                updateActionModePreferenceDescription(dispid, mKeepRatePref[dispid].getEntry());

                break;
            }
        }
        
        return true;
    }

    private void addDisplayPreference(int fbid) {
        String dispKey = null;
        String dispTitle = null;
        String dispSummary = null;
        String dispDialogTitle = null;
        mCategoryPref[fbid] = new PreferenceCategory(getActivity());

        dispKey = "display_category_" + fbid;
        mCategoryPref[fbid].setKey(dispKey);
        mCategoryPref[fbid].setOnPreferenceChangeListener(this);
        getPreferenceScreen().addPreference(mCategoryPref[fbid]);

        if(fbid != 0) {
            mDisplayEnablePref[fbid] = new CheckBoxPreference(getActivity());

            dispKey = "display_enable_" + fbid;
            mDisplayEnablePref[fbid].setKey(dispKey);
            dispTitle = getActivity().getString(R.string.pluggable_settings_enable_display);//"Enable Display";
            mDisplayEnablePref[fbid].setTitle(dispTitle);

            mDisplayEnablePref[fbid].setOnPreferenceChangeListener(this);
            mCategoryPref[fbid].addPreference(mDisplayEnablePref[fbid]);
        }

        mDisplayModePref[fbid] = new ListPreference(getActivity());

        dispKey = "display_mode_" + fbid;
        mDisplayModePref[fbid].setKey(dispKey);
        dispTitle = getActivity().getString(R.string.pluggable_settings_display_mode);//"Display Mode";
        mDisplayModePref[fbid].setTitle(dispTitle);
        dispSummary = "No display plugged";
        mDisplayModePref[fbid].setSummary(dispSummary);
        dispDialogTitle = getActivity().getString(R.string.pluggable_settings_display_mode);//"Display Mode";
        mDisplayModePref[fbid].setDialogTitle(dispDialogTitle);

        mDisplayModePref[fbid].setOnPreferenceChangeListener(this);
        mCategoryPref[fbid].addPreference(mDisplayModePref[fbid]);

        mColorDepthPref[fbid] = new ListPreference(getActivity());

        dispKey = "display_colordepth_" + fbid;
        mColorDepthPref[fbid].setKey(dispKey);
        dispTitle = getActivity().getString(R.string.pluggable_settings_colordepth);//"Color Depth";
        mColorDepthPref[fbid].setTitle(dispTitle);
        dispSummary = "Color Depth";
        mColorDepthPref[fbid].setSummary(dispSummary);
        dispDialogTitle = getActivity().getString(R.string.pluggable_settings_colordepth);//"Color Depth";
        mColorDepthPref[fbid].setDialogTitle(dispDialogTitle);

        mColorDepthPref[fbid].setOnPreferenceChangeListener(this);
        mCategoryPref[fbid].addPreference(mColorDepthPref[fbid]);

    }

    private int getDisplayColorEntry(int dispid) {
        int entry = -1;

        switch(dispid) {
            case 0:
                entry = R.array.entries_display_colordepth_0;
                break;
            case 1:
                entry = R.array.entries_display_colordepth_1;
                break;
            case 2:
                entry = R.array.entries_display_colordepth_2;
                break;
            case 3:
                entry = R.array.entries_display_colordepth_3;
                break;
            case 4:
                entry = R.array.entries_display_colordepth_4;
                break;
            case 5:
                entry = R.array.entries_display_colordepth_5;
                break;
        }
        return entry;
    }

    private int getDisplayColorEntryValue(int dispid) {
        int entryVal = -1;

        switch(dispid) {
            case 0:
                entryVal = R.array.entryvalues_display_colordepth_0;
                break;
            case 1:
                entryVal = R.array.entryvalues_display_colordepth_1;
                break;
            case 2:
                entryVal = R.array.entryvalues_display_colordepth_2;
                break;
            case 3:
                entryVal = R.array.entryvalues_display_colordepth_3;
                break;
            case 4:
                entryVal = R.array.entryvalues_display_colordepth_4;
                break;
            case 5:
                entryVal = R.array.entryvalues_display_colordepth_5;
                break;
        }
        return entryVal;
    }

    private void handleDisplayConnected(int dispid, boolean connection){
        String[] display_modes;
        if(DBG) Log.w(TAG, "handleDisplayConnected " + connection + "dispid "+ dispid);
        if (dispid < 0) {
            Log.w(TAG, "dispid is no valid");
            return;
        }

        if(connection) {
            String displayName = null;
            String displayType = null;
            boolean isHDMI = false;

            displayType = mDisplayManager.getDisplayName(dispid);
            if(displayType != null) {
                isHDMI = displayType.contains("hdmi");
            }

            if(dispid == 0) {
                displayType = getActivity().getString(R.string.pluggable_settings_primary_display);
            } else {
                displayType = getActivity().getString(R.string.pluggable_settings_added_dislay);
            }

            if(isHDMI) {
                displayName = displayType + ": " + getActivity().getString(R.string.pluggable_settings_display_hdmi);
            }
            else {
                displayName = displayType + ": " + getActivity().getString(R.string.pluggable_settings_display_ldb);
            }


            if((mCategoryPref[dispid] == null)) {
                addDisplayPreference(dispid);
            }

            if((mCategoryPref[dispid] == null) || (mDisplayModePref[dispid] == null)
                  || (mColorDepthPref[dispid] == null)) {
                Log.w(TAG, "addDisplayPreference init failed");
                return;
            }

            getPreferenceScreen().addPreference(mCategoryPref[dispid]);
            mCategoryPref[dispid].setTitle(displayName);

            String currentDisplayMode = mDisplayManager.getDisplayMode(dispid);
            display_modes = mDisplayManager.getDisplayModeList(dispid);
            if(display_modes == null) {
                Log.e(TAG, "display_modes is no valid");
                return;
            }
            mHighMode[dispid] = display_modes[0];
            mCurrentMode[dispid] = currentDisplayMode;
            
            ArrayList<CharSequence> revisedEntries = new ArrayList<CharSequence>();
            ArrayList<CharSequence> revisedValues = new ArrayList<CharSequence>();

            revisedEntries.add(getActivity().getString(R.string.pluggable_settings_keep_highest_mode));
            revisedValues.add(DISPLAY_HIGH_MODE);

            for (String imode : display_modes) {
                revisedEntries.add(imode);
                revisedValues.add(imode);
            }

            mDisplayModePref[dispid].setEntries(
                revisedEntries.toArray(new CharSequence[revisedEntries.size()]));
            mDisplayModePref[dispid].setEntryValues(
                    revisedValues.toArray(new CharSequence[revisedValues.size()]));

            mDisplayModePref[dispid].setValue(String.valueOf(currentDisplayMode));
            mDisplayModePref[dispid].setOnPreferenceChangeListener(this);
            updateDisplayModePreferenceDescription(dispid, currentDisplayMode);

            int entry = getDisplayColorEntry(dispid);
            int entryVal = getDisplayColorEntryValue(dispid);
            if(entry < 0 || entryVal < 0) {
                mColorDepthPref[dispid].setEnabled(false);
            } else {
                mColorDepthPref[dispid].setEntries(entry);
                mColorDepthPref[dispid].setEntryValues(entryVal);
                int currentDisplayColorDepth = mDisplayManager.getDisplayColorDepth(dispid);
                mColorDepthPref[dispid].setValue(String.valueOf(currentDisplayColorDepth));
                mColorDepthPref[dispid].setOnPreferenceChangeListener(this);
                updateDisplayColorDepthPreferenceDescription(dispid, currentDisplayColorDepth);       
            }
            if(isHDMI) {
                if(mXOverScanPref[dispid] == null) {
                    mXOverScanPref[dispid] = new SeekBarPreference(getActivity());
                    String dispStr = "display_xoverscan_" + dispid;
                    mXOverScanPref[dispid].setKey(dispStr);
                    dispStr = getActivity().getString(R.string.pluggable_settings_width_overscan);//"Width OverScan";
                    mXOverScanPref[dispid].setTitle(dispStr);
                    int value = 30;
                    mXOverScanPref[dispid].setMax(value);
                    value = mDisplayManager.getDisplayXOverScan(dispid);
                    mXOverScanPref[dispid].setProgress(value);
                    mXOverScanPref[dispid].setPersistent(false);

                    mXOverScanPref[dispid].setOnPreferenceChangeListener(this);
                    mCategoryPref[dispid].addPreference(mXOverScanPref[dispid]);
                }
                if(mYOverScanPref[dispid] == null) {
                    mYOverScanPref[dispid] = new SeekBarPreference(getActivity());
                    String dispStr = "display_yoverscan_" + dispid;
                    mYOverScanPref[dispid].setKey(dispStr);
                    dispStr = getActivity().getString(R.string.pluggable_settings_height_overscan);//"Height OverScan";
                    mYOverScanPref[dispid].setTitle(dispStr);
                    int value = 30;
                    mYOverScanPref[dispid].setMax(value);
                    value = mDisplayManager.getDisplayYOverScan(dispid);
                    mYOverScanPref[dispid].setProgress(value);
                    mYOverScanPref[dispid].setPersistent(false);

                    mYOverScanPref[dispid].setOnPreferenceChangeListener(this);
                    mCategoryPref[dispid].addPreference(mYOverScanPref[dispid]);
                }

		if(mKeepRatePref[dispid] == null) {
		    String dispKey = null;
		    String dispTitle = null;
		    String dispSummary = null;
		    String dispDialogTitle = null;

		    mKeepRatePref[dispid] = new ListPreference(getActivity());

		    dispKey = "display_keeprate_" + dispid;
		    mKeepRatePref[dispid].setKey(dispKey);
		    dispTitle = getActivity().getString(R.string.pluggable_settings_aspect_ratio);//"Aspect Ratio";
		    mKeepRatePref[dispid].setTitle(dispTitle);
		    dispSummary = "aspect ration";
		    mKeepRatePref[dispid].setSummary(dispSummary);
		    dispDialogTitle = getActivity().getString(R.string.pluggable_settings_aspect_ratio);//"Aspect Ratio";
		    mKeepRatePref[dispid].setDialogTitle(dispDialogTitle);

		    mKeepRatePref[dispid].setOnPreferenceChangeListener(this);
		    mCategoryPref[dispid].addPreference(mKeepRatePref[dispid]);

		    if(dispid == 0) {
			entry = R.array.entries_primary_action_mode;
			entryVal = R.array.entryvalues_primary_action_mode;
		    } else {
			entry = R.array.entries_action_mode;
			entryVal = R.array.entryvalues_action_mode;
		    }
		    if(entry < 0 || entryVal < 0) {
			mKeepRatePref[dispid].setEnabled(false);
		    } else {
			mKeepRatePref[dispid].setEntries(entry);
			mKeepRatePref[dispid].setEntryValues(entryVal);
			int keepRate = mDisplayManager.getDisplayKeepRate(dispid);
			mKeepRatePref[dispid].setValue(Integer.toHexString(keepRate));
			updateActionModePreferenceDescription(dispid, mKeepRatePref[dispid].getEntry());
		    }
		}


            } else {
                mColorDepthPref[dispid].setEnabled(false);
            }

            if(dispid != 0) {
                if((mDisplayEnablePref[dispid] == null)) {
                    Log.w(TAG, "addDisplayPreference init 2 failed");
                    return;
                }

                boolean currentDisplayEnable = mDisplayManager.getDisplayEnable(dispid);
                if(DBG) Log.w(TAG,"currentDisplayEnable " +currentDisplayEnable);
                mDisplayEnablePref[dispid].setChecked(currentDisplayEnable);
                mDisplayEnablePref[dispid].setOnPreferenceChangeListener(this);

                if(mColorDepthPref[dispid] != null && isHDMI)
                    mColorDepthPref[dispid].setEnabled(true);
            }
        } else {
            // delete the preferenc entry and value;
            if((mDisplayModePref[dispid] == null) || (mCategoryPref[dispid] == null)) {
                Log.w(TAG, "addDisplayPreference init 3 failed");
                return;
            }

            ArrayList<CharSequence> revisedEntries = new ArrayList<CharSequence>();
            ArrayList<CharSequence> revisedValues = new ArrayList<CharSequence>();              
            mDisplayModePref[dispid].setEntries(
                revisedEntries.toArray(new CharSequence[revisedEntries.size()]));
            mDisplayModePref[dispid].setEntryValues(
                    revisedValues.toArray(new CharSequence[revisedValues.size()]));     
            mDisplayModePref[dispid].setOnPreferenceChangeListener(this);

            getPreferenceScreen().removePreference(mCategoryPref[dispid]);
        }
    }
    
    private void handleDisplayStateChanged(int dispid, int state) {
        if(DBG) Log.w(TAG, "handleDisplayStateChanged");
        if (dispid < 0) {
            Log.w(TAG, "dispid is no valid");
            return;
        }
        
    }

    private void showRebootDialog() {
    
        CharSequence msg = getResources().getText(R.string.alarm_reboot_msg);
        mConfirmDialog = new AlertDialog.Builder(getActivity()).setMessage(msg)
                .setTitle(R.string.alarm_reboot_title)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setPositiveButton(android.R.string.ok, this)
                .setNegativeButton(android.R.string.cancel, this)
                .show();
    }

    public void onClick(DialogInterface dialog, int which) {
        if (which == DialogInterface.BUTTON_POSITIVE) {
            Log.w(TAG," changed the setting, ok to reboot");
            mDisplayManager.rebootSystem();
        } else {
            Log.w(TAG," changed the setting, but cancel to reboot");
        }
    }
        
}


