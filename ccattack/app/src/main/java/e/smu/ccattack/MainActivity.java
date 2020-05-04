package e.smu.ccattack;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import e.smu.ccattack.R;

import android.Manifest;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {
    int pid;
    int cpu=0;
    String command = "";
    int fork=1;
    String[] ranges;
    String[] offsets;
    String[] filenames;
    String[] func_lists;
    String target_lib = "libcamera_client.so"; //The monitored function of library
    String target_func = "4ac7c";  // The offset of the monitored function.
    ArrayList<Integer> ids = new ArrayList<Integer>();
    public final String TAG = "MainActivity";
    static {
        System.loadLibrary("native-lib"); //jni lib to use libflush
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        pid = android.os.Process.myPid(); //get the self pid
        ArrayList<String> range = new ArrayList<String>(); //the addresses of libraries in memory;
        ArrayList<String> offset = new ArrayList<String>(); //the offsets of libraries;

        ArrayList<String> filename = new ArrayList<String>();
        ArrayList<String> func_list = new ArrayList<String>();
        ArrayList<String> targets = new ArrayList<String>(); //filenames in assets
        //get all filenames in assets
        AssetManager am = getAssets();
        try {
            for(String f:am.list("")){
                if(f.equals(target_lib))// move the target library to the head of the list.
                {
                    targets.add(0, f);
                    continue;
                }
                if(f.substring(f.lastIndexOf(".")+1).equals("so")){
                    targets.add(f); //
                    Log.d("MainActivity",f+" "+f.substring(f.lastIndexOf(".")+1));
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        //get corresponding ranges, offsets, etc;
        for(int i=0;i<targets.size();i++) {
            Log.d("MainActivity",i+" "+targets.get(i));
            String[] arr = exec(pid, targets.get(i));
            range.add(arr[0]);
            offset.add(arr[2]);
            filename.add(arr[arr.length - 1]); //get the path of library in the android system;
            func_list.add(readSaveFile(targets.get(i)));//read addresses in files
            //get target
            if(targets.get(i).equals(target_lib))
                target_lib = range.get(i).split("-")[0];//replace lib string with its address in memory;
        }
        //convert into String[]
        ranges = (String[])range.toArray(new String[targets.size()]);
        offsets = (String[])offset.toArray(new String[targets.size()]);
        filenames = (String[])filename.toArray(new String[targets.size()]);
        func_lists = (String[])func_list.toArray(new String[targets.size()]);

        Log.d(TAG,"target:"+target_func+" "+target_lib);
        Button button = (Button) findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread() {
                    @Override
                    public void run() {
                        scan(cpu,ranges,offsets,fork,filenames,func_lists,target_lib,target_func);//start scan
                    }
                }.start();
            }
        });

        Button button1 = (Button) findViewById(R.id.button1);
        button1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread() {
                    @Override
                    public void run() {
                        access(target_lib,target_func);//access the address of the function
                    }
                }.start();
            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String scan(int cpu, String[] range, String[] offset, int fork, String[] filename,String[] func_list,String target_lib, String target_func);
    public native void access(String x,String y);

    public String[] exec(int pid,String target) {
        String data="";
        try {
            Process p = null;
            String command = "grep "+target+" /proc/"+pid+"/maps";
            //Log.d("MainActivity",command);
            p = Runtime.getRuntime().exec(command);
            BufferedReader ie = new BufferedReader(new InputStreamReader(p.getErrorStream()));
            BufferedReader in = new BufferedReader(new InputStreamReader(p.getInputStream()));
            String error = null;
            while ((error = ie.readLine()) != null
                    && !error.equals("null")) {
                data += error + "\n";
            }
            String line = null;
            while ((line = in.readLine()) != null
                    && !line.equals("null")) {
                data += line + "\n";
            }
            String[] arr = data.split("\n");
            for(String st:arr){
                if(st.contains("-xp ")&&st.split("/").length==4){
                    Log.d(TAG,st);
                    data = st;
                    break;}
            }
            //Log.v("MainActivity", data);
            ;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return data.split(" ");
    }


    private String readSaveFile(String filename) {
        InputStream inputStream;
        try {
            inputStream = getResources().getAssets().open(filename);
            byte temp[] = new byte[1024];
            StringBuilder sb = new StringBuilder("");
            int len = 0;
            while ((len = inputStream.read(temp)) > 0){
                sb.append(new String(temp, 0, len));
            }
            //Log.d("msg", "readSaveFile: \n" + sb.toString());
            inputStream.close();
            return sb.toString();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return "";
    }
}

