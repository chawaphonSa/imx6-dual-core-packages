/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.android.contacts.tests.testauth;

import android.accounts.AbstractAccountAuthenticator;
import android.accounts.Account;
import android.accounts.AccountAuthenticatorResponse;
import android.accounts.AccountManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * Simple authenticator.  It has no "login" dialogs/activities.  When you add a new account, it'll
 * just create a new account with a unique name.
 */
class TestAuthenticator extends AbstractAccountAuthenticator {
    private static final String PASSWORD = "xxx"; // any string will do.

    // To remember the last user-ID.
    private static final String PREF_KEY_LAST_USER_ID = "TestAuthenticator.PREF_KEY_LAST_USER_ID";

    private final Context mContext;

    public TestAuthenticator(Context context) {
        super(context);
        mContext = context.getApplicationContext();
    }

    /**
     * @return a new, unique username.
     */
    private String newUniqueUserName() {
        final SharedPreferences prefs =
                PreferenceManager.getDefaultSharedPreferences(mContext);
        final int nextId = prefs.getInt(PREF_KEY_LAST_USER_ID, 0) + 1;
        prefs.edit().putInt(PREF_KEY_LAST_USER_ID, nextId).apply();

        return "User-" + nextId;
    }

    /**
     * Create a new account with the name generated by {@link #newUniqueUserName()}.
     */
    @Override
    public Bundle addAccount(AccountAuthenticatorResponse response, String accountType,
            String authTokenType, String[] requiredFeatures, Bundle options) {
        Log.v(TestauthConstants.LOG_TAG, "addAccount() type=" + accountType);
        final Bundle bundle = new Bundle();

        final Account account = new Account(newUniqueUserName(), accountType);

        // Create an account.
        AccountManager.get(mContext).addAccountExplicitly(account, PASSWORD, null);

        // And return it.
        bundle.putString(AccountManager.KEY_ACCOUNT_NAME, account.name);
        bundle.putString(AccountManager.KEY_ACCOUNT_TYPE, account.type);
        return bundle;
    }

    /**
     * Just return the user name as the authtoken.
     */
    @Override
    public Bundle getAuthToken(AccountAuthenticatorResponse response, Account account,
            String authTokenType, Bundle loginOptions) {
        Log.v(TestauthConstants.LOG_TAG, "getAuthToken() account=" + account);
        final Bundle bundle = new Bundle();
        bundle.putString(AccountManager.KEY_ACCOUNT_NAME, account.name);
        bundle.putString(AccountManager.KEY_ACCOUNT_TYPE, account.type);
        bundle.putString(AccountManager.KEY_AUTHTOKEN, account.name);

        return bundle;
    }

    @Override
    public Bundle confirmCredentials(
            AccountAuthenticatorResponse response, Account account, Bundle options) {
        Log.v(TestauthConstants.LOG_TAG, "confirmCredentials()");
        return null;
    }

    @Override
    public Bundle editProperties(AccountAuthenticatorResponse response, String accountType) {
        Log.v(TestauthConstants.LOG_TAG, "editProperties()");
        throw new UnsupportedOperationException();
    }

    @Override
    public String getAuthTokenLabel(String authTokenType) {
        // null means we don't support multiple authToken types
        Log.v(TestauthConstants.LOG_TAG, "getAuthTokenLabel()");
        return null;
    }

    @Override
    public Bundle hasFeatures(
            AccountAuthenticatorResponse response, Account account, String[] features) {
        // This call is used to query whether the Authenticator supports
        // specific features. We don't expect to get called, so we always
        // return false (no) for any queries.
        Log.v(TestauthConstants.LOG_TAG, "hasFeatures()");
        final Bundle result = new Bundle();
        result.putBoolean(AccountManager.KEY_BOOLEAN_RESULT, false);
        return result;
    }

    @Override
    public Bundle updateCredentials(AccountAuthenticatorResponse response, Account account,
            String authTokenType, Bundle loginOptions) {
        Log.v(TestauthConstants.LOG_TAG, "updateCredentials()");
        return null;
    }
}
