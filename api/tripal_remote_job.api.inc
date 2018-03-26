<?php

/**
 * Adds a job containing a system command for execution on a remote resource.
 *
 * The command must be executable on the remote resource.  This means that
 * all necessary binaries and environment settings should be ready to go.  When
 * a job is run on a remote resource a unique execution directory is created
 * on the remote system and the job is executed there.  The job will be
 * scheduled on any remote resources that has been configured and that has
 * available slots.
 *
 * This function adds a job to the Tripal Jobs systems such that a remote job
 * is only executed when the Job's system runs jobs.  The Tripal Daemon can
 * be used to ensure jobs run almost as soon as they are submitted.
 *
 * After execution, results can be found here:
 *
 * public://tripal/jobs/{job_id}
 *
 * Where 'public://' is the location of Drupal's public files.
 *
 * @param $command
 *   The system command to execute.
 * @param unknown $files
 *   A list of files that must be copied to the remote host prior to
 *   execution.
 * @return
 *  The Tripal Job ID number for the requested job
 */
function tripal_add_remote_job($job_name, $command, $files = array(), $send_email = FALSE){
  global $user;
  
  $arguments = array($command, $files, $send_email);

  return tripal_add_job($job_name, 'tripal_remote_job', 'tripal_remote_job_run_job',
      $arguments, $user->uid);
}
/*
 * @param $job_id
 *  Tripal Job ID number
 * 
 * @return
 *  An integer or string representing the specified job's status
 * 
 * @todo
 *  Complete functionalitys
 *  Define how the return value will be held (string, int, etc.)
 */
function check_job_progress($job_id)
{
    $job = tripal_get_job($job_id);
    
    dpm($job);
    echo $job_id." = ".$job;
    return $job;
}
/*
 * Check to see if a file is readable by the site (webserver/php)
 * @param $filesToCheck
 *  An array of files to be checked on the remote system
 * 
 * @return
 *  An array with matching $filesToCheck with 0 or 1 (for accessibility)
 *  of the maching file (based on array index)
 * 
 * @todo
 *  Run this check without spawning a Tripal Job (or else it may clutter
 *  up the Tripal Job queue)
 */
function tripal_remote_check_files($filesToCheck)
{
    
    $accessible = array();
    for ($k = 0; $k < count($filesToCheck);$k++)
    {
        if ($k % 2 == 0) {
            $accessible[$k] = 0;
        }
        else
            $accessible[$k] = 1;
        
    }
    return $accessible;
}
