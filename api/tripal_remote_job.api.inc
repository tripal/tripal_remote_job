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
function tripal_add_remote_job($job_name, $command, $origin_module, $files = array(), $send_email = FALSE){
  global $user;
  
  $arguments = array($command." & echo $! > PID", $files, $send_email);
  
  if ($origin_module)
  {
      $module = $origin_module;              
  }
  else
  {
      $module = 'tripal_remote_job';
  }

  return tripal_add_job($job_name, $module, 'tripal_remote_job_run_job',
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
 * @param $resource_id
 *  ID of the Tripal Remote Resource - use to determine which method to check
 * 
 * @return
 *  An array with matching $filesToCheck with 0 or 1 (for accessibility)
 *  of the maching file (based on array index)
 * 
 * @todo
 *  Run this check without spawning a Tripal Job (or else it may clutter
 *      up the Tripal Job queue)
 *  Make this decide how to check based on resource type, then delegate to that
 *      class (SSH for instance)
 */
function tripal_remote_check_files($filesToCheck, $resource_id)
{
    module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteResource');
    module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteSSH');
    
    if (!$resource_id) {
        $sql = "SELECT resource_id FROM {tripal_remote_resource} where enabled='1' ORDER BY rank ASC LIMIT 1";
        $sqlResult = db_query($sql);
        $resource_id = $sqlResult->fetchField();
    }
    
    $res = TripalRemoteResource::getResource($resource_id);
    if ($res) {
      // Get the resource type.
      $resource_type = $res->getType();
    }
    else {
        return NULL;
    }
    
    switch ($resource_type) {
        case 'alone' :
          $accessible = $res->checkFiles($filesToCheck);
          break;
    }
            
    return $accessible;
}


/*
 * Retrieve a TripalRemoteResource object item for viewing details.
 * Does not initiate connection to the remote resource
 * 
 * @param $connection_ID
 *  An integer ID belonging to an existing TRJ Connection
 * 
 * @return
 *  Connection object, false on error.
 * 
 * @todo
 *  Currently only will support SSH (TRJ only supports SSH)
 *  Implement others (they all extend TripalRemoteResource)
 */
function tripal_remote_job_get_resource_details($resource_ID)
{
    module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteResource');
    module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteSSH');
    #$resource = TripalRemoteResource::byID($resource_ID);
    
    $resourceForType = TripalRemoteResource::byID($resource_ID);
    
    
    
    $resource = TripalRemoteResource::getResource($resource_ID);
    
    if ($resource->getType() == "alone") {
        echo "Hostname: ".$resource->getHostname()."\n";
        echo "Port: ".$resource->getSSHPort()."\n"; 
        echo "Type: ".$resource->getType()."\n"; 
    }
    else {
        "Resource specified is not an SSH/standalone server. Not sure how to proceed";
    }
    
    
}


/*
 * Check whether form data in the add/edit remote resource page is correct
 * The connection details are passed to the function directly because these
 * values may not exist in the database yet (user will either have not submitted
 * them yet or have altered values in the form).
 * 
 * @param $connection_details
 *  Array containing information about the server:
 *      hostname
 *      port
 *      username
 *      remote working directory
 * 
 * @return
 *  connection status, possible conditions:
 * 0 - Connection successful, remote working directory exists and is writeable    [directory_exists_is_writable]
 * 1 - Connection successful, remote working directory exists but is not writable [directory_exists_not_writable]
 * 2 - Connection successful, remote working directory did not exist, created     [directory_created]
 * 3 - Connection successful, remote working directory could not be created       [directory_failed]
 * 4 - Connection unsuccessful
 * 5 - Something went wrong prior to connection
 *      
 * @todo
 *  Currently only supports SSH configurations
 *  Implement other 
 *  SSH-specific: make compatible with other shells as needed (with functionality
 *                to detect the available remote shell)
 */
function tripal_remote_job_check_configuration($connection_details)
{
    // Generate the command to test the connection.
    $rwd = $connection_details['remote_working_directory'];
    $cmd = 'ssh -p '.$connection_details['ssh_port'].' -i '.$connection_details['ssh_key_location'].' '.$connection_details['username'].'@'.$connection_details['hostname'].' \' if [ -e '.$rwd.' ]; then if [ -w '.$rwd.' ]; then echo "directory_exists_is_writable"; else echo "directory_exists_not_writable"; fi else mkdir -p '.$rwd.'; if [ $? -eq  0 ]; then echo "directory_created"; else echo "directory_failed"; fi fi \'';
    
    //drupal_set_message($cmd);
    exec($cmd, $results, $return_var);
    //dpm($results[0]);
    //dpm($return_var);
    
    $status_code = 5;
    // $return_var will be non-zero if there was an error with the connection
    if ($return_var)
    {
        $status_code = 4;
    }
    // Otherwise, see how the server configuration went after connecting
    else
    {
        switch ($results[0])
      {
          case "directory_exists_is_writable":
              $status_code = 0;
              break;
          case "directory_exists_not_writable":
              $status_code = 1;
              break;
          case "directory_created":
              $status_code = 2;
              break;
          case "directory_failed":
              $status_code = 3;
              break;          
      }
    }
    
    return $status_code;
}