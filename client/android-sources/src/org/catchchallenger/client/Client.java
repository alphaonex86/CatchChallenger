package org.catchchallenger.client;

import android.content.Context;

public class Client extends org.qtproject.qt5.android.bindings.QtActivity {
    private static Context context;
    private static boolean firstTime;

    public Client() {
        firstTime = true;
    }

    @Override
    public void onCreate(android.os.Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        context = getApplicationContext();
    }

    @Override
    public void onResume() {
        super.onResume();
        if (!firstTime) {
            JniMessenger.onEventApplication("resume");
        }
        firstTime = false;
    }

    @Override
    public void onPause() {
        super.onPause();
        if (!firstTime) {
            JniMessenger.onEventApplication("pause");
        }
    }

    public static Context getContext() {
        return context;
    }
}
