package e.smu.ccattack;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import e.smu.ccattack.R;

import android.Manifest;
import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

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
    static String[] ranges;
    static String[] offsets;
    static String[] filenames;
    static String[] func_lists;
    static String target_lib = "libcamera_client.so"; //The monitored function of library
    static String target_func = "4ac7c";  // The offset of the monitored function.
    ArrayList<Integer> ids = new ArrayList<Integer>();
    public final String TAG = "MainActivity";
    EditText jobStatus;
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
        //Switch
        final Switch mSwitch = (Switch) findViewById(R.id.btn_schedule_job);
        jobStatus = (EditText) findViewById(R.id.job_status);
        // 添加监听 scheduleJob cancelJob
        mSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    scheduleJob(buttonView);
                } else {
                    cancelJob(buttonView);
                }
            }
        });
        Button button = (Button) findViewById(R.id.buttonr);
        button.setOnClickListener(new View.OnClickListener() {
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
    public native void access(String x,String y);
    /**
     * Method to check if the job scheduler is running
     *
     * @param isRunning: flag to indicate if the job scheduler is running
     */
    public void checkRunStatus(boolean isRunning) {
        if (isRunning) {
            jobStatus.setText("RUNNING");
        } else {
            jobStatus.setText("STOPPED");
        }
    }

    /**
     * Method to start the job scheduler
     * @param v: the current view at which this method is called
     */
    public void scheduleJob(View v) {
        // Building the job to be passed to the job scheduler
        ComponentName componentName;
        componentName = new ComponentName(this, ScanService.class);
        ScanService.continueRun = true;
        checkRunStatus(ScanService.continueRun);
        JobInfo info = new JobInfo.Builder(111, componentName)
                .setRequiresCharging(false)
                .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
                .setPersisted(false)
                .setOverrideDeadline(0)
                .build();
        // Creating the job scheduler
        JobScheduler scheduler = (JobScheduler) getSystemService(JOB_SCHEDULER_SERVICE);
        int resultCode = scheduler.schedule(info);
        if (resultCode == JobScheduler.RESULT_SUCCESS) {
            Log.d(TAG, "Job scheduled");
            Toast.makeText(this, "Job scheduled successfully", Toast.LENGTH_SHORT)
                    .show();
        } else {
            Log.d(TAG, "Job scheduling failed");
            Toast.makeText(this, "Job scheduling failed", Toast.LENGTH_SHORT)
                    .show();
        }
    }

    /**
     * Method to stop the job scheduler
     * @param v: the current view at which this method is called
     */
    public void cancelJob(View v) {
        JobScheduler scheduler = (JobScheduler) getSystemService(JOB_SCHEDULER_SERVICE);
        scheduler.cancel(111);
        ScanService.continueRun = false;
        checkRunStatus(ScanService.continueRun);
        Log.d(TAG, "Job cancelled");
    }

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

