package org.catchchallenger.client;

import android.content.Context;

public class Client extends org.qtproject.qt5.android.bindings.QtActivity
{
    private static Context context;

    public Client()
    {
    }

    @Override
    public void onCreate(android.os.Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        context = getApplicationContext();
    }

    public static Context getContext()
    {
        return context;
    }
}
