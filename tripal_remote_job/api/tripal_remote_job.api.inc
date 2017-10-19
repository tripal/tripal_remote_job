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
 */
function tripal_add_remote_job($job_name, $command, $files = array(), $send_email = FALSE){
  global $user;
  
  $arguments = array($command, $files, $send_email);

  return tripal_add_job($job_name, 'tripal_remote_job', 'tripal_remote_job_run_job',
      $arguments, $user->uid);
}
