package e.smu.ccattack;

import android.app.job.JobParameters;
import android.app.job.JobService;
import android.util.Log;
import android.widget.Toast;

import static e.smu.ccattack.MainActivity.filenames;
import static e.smu.ccattack.MainActivity.func_lists;
import static e.smu.ccattack.MainActivity.offsets;
import static e.smu.ccattack.MainActivity.ranges;
import static e.smu.ccattack.MainActivity.target_func;
import static e.smu.ccattack.MainActivity.target_lib;


public class ScanService extends JobService {
    public static boolean continueRun;
    private static final String TAG = "JobService";
    int scValueCount = 1;
    int index =1;
    long cumulativeTime;
    Thread mthread;

    @Override
    public boolean onStartJob(JobParameters params) {
        Log.d(TAG, "Job started");
        Toast.makeText(this, "Started Data Collection", Toast.LENGTH_SHORT).show();
        doBackgroundWork(params);
        return true;
    }

    /**
     * Method to run data collection in the background
     *
     * @param params: parameters passed into the job scheduler from MainActivity
     */
    public native String scan(int cpu, String[] range, String[] offset, int fork, String[] filename,String[] func_list,String target_lib, String target_func);
    public native void pause();

    public void doBackgroundWork(final JobParameters params) {
        Log.d(TAG, "New Thread Created");
        // Send the job to a different thread
        mthread = new Thread(new Runnable() {
            @Override
             public void run() {
                scan(0,ranges,offsets,1,filenames,func_lists,target_lib,target_func);//start scan
                }
            });
        mthread.start();
    }

    @Override
    public boolean onStopJob(JobParameters params) {
        Log.d(TAG, "Job cancelled before completion");

        continueRun = false;
        pause();//pause the scanning
        Toast.makeText(this, "Job cancelled", Toast.LENGTH_SHORT)
                    .show();
        return true;
    }

}