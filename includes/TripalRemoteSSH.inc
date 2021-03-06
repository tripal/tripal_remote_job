<?php

class TripalRemoteSSH extends TripalRemoteResource {

  // TRJobRemoteResource data members.
  protected $resource_id;
  protected $hostname;
  protected $ssh_port;
  protected $username;
  protected $ssh_key_location;

  /**
   * Implements the class constructor.
   *
   * @param $details
   */
  public function __construct($details = array()) {
    $this->resource_id = array_key_exists('resource_id', $details)  ? $details['resource_id']  : '';
    $this->hostname = array_key_exists('hostname', $details)  ? $details['hostname']  : '';
    $this->ssh_port = array_key_exists('ssh_port', $details)  ? $details['ssh_port']  : 22;
    $this->username = array_key_exists('username', $details)  ? $details['username']  : '';
    $this->ssh_key_location  = array_key_exists('ssh_key_location', $details)   ? $details['ssh_key_location']   : '~/.ssh/id_rsa';

    parent::__construct($details);
  }

  /**
   * Implements the class destructor.
   */
  public function __destruct() {
    parent::__destruct();
  }

  /**
   * Create TRJobRemoteResource object by provided ID.
   *
   * @param $resource_id
   *
   * @return TRJobRemoteResource|NULL
   */
  public static function byID($resource_id) {

    // Get the resource info.
    $sql = "SELECT * FROM {tripal_remote_resource} WHERE resource_id = :resource_id";
    $resource = db_query($sql, array(':resource_id' => $resource_id))->fetch(PDO::FETCH_ASSOC);

    // Add connection info.
    $sql = "SELECT * FROM {tripal_remote_ssh} WHERE resource_id = :resource_id";
    $ssh_resource = db_query($sql, array(':resource_id' => $resource_id))->fetch(PDO::FETCH_ASSOC);
    $resource['resource_id'] = $ssh_resource['resource_id'];
    $resource['hostname'] = $ssh_resource['hostname'];
    $resource['ssh_port'] = $ssh_resource['ssh_port'];
    $resource['username'] = $ssh_resource['username'];
    $resource['ssh_key_location'] = $ssh_resource['ssh_key_location'];

    // Return the instance of this class.
    if ($resource) {
      return new self($resource);
    }
    else {
      return NULL;
    }
  }

  public function checkFiles($filesToCheck)
  {
      $cmdA = "ssh -p $this->ssh_port -i $this->$ssh_key_location $this->username@$this->hostname";
      $results = array();
      foreach ($filesToCheck as $files)
      {   
          $cmdB = "$cmdA [[ -r $files ]] && echo 1 || echo 0";
          echo $cmdB;
          exec($cmdB,$results);
      }
      /*foreach ($results as $key => $result) {
          echo "$filesToCheck[$key] $result <br>";
      } *///Debuggery
      return $results;
  }
  /**
   * Adds a new resource submit.
   *
   * @return boolean TRUE|FLASE
   */
  public function insert() {
    try {

      parent::insert();

      // Check for duplication (resource_id and hostname).
      $sql = "
        SELECT resource_id
        FROM {tripal_remote_ssh}
        WHERE resource_id = :resource_id AND hostname = :hostname
      ";
      $args = array(
        ':resource_id'  => $this->resource_id,
        ':hostname'     => $this->hostname
      );
      $resource_id = db_query($sql, $args)->fetchField();
      if($resource_id) {
        throw new Exception("The resource submit (resource_id and hostname) already exists");
      }

      // Insert a new resource submit.
      db_insert('tripal_remote_ssh')
        ->fields(array(
          'resource_id' => $this->resource_id,
          'hostname'    => $this->hostname,
          'ssh_port'    => $this->ssh_port,
          'username'    => $this->username,
          'ssh_key_location'    => $this->ssh_key_location,
        ))
        ->execute();
    }
    catch (Exception $e) {
      drupal_set_message('Error inserting resource submit');
      watchdog('tripal_remote_job', $e->getMessage(), array(), WATCHDOG_ERROR);
      return FALSE;
    }
    return TRUE;
  }

  /**
   * Updates the resource submit.
   *
   * @return boolean TURE|FALSE
   */
  public function update() {

    // Update the resource submit.
    try {
      parent::update();
      db_update('tripal_remote_ssh')
        ->fields(array(
          'resource_id' => $this->resource_id,
          'hostname'    => $this->hostname,
          'ssh_port'    => $this->ssh_port,
          'username'    => $this->username,
          'ssh_key_location'   => $this->ssh_key_location,
        ))
        ->condition('resource_id', $this->resource_id, '=')
        ->execute();
    }
    catch (Exception $e) {
      drupal_set_message('Error updating resource submit');
      watchdog('tripal_remote_job', $e->getMessage(), array(), WATCHDOG_ERROR);
      return FALSE;
    }
    return TRUE;
  }

  /**
   * @see TripalRemoteResource::getFreeSlots()
   */
  public function getFreeSlots(){
    $jobs = $this->getRunningJobs();
    $num_jobs = count($jobs);

    return max(array($this->max_slots - $num_jobs, 0));
  }

  /**
   * @see TripalRemoteResource::getRunningJobs()
   */
  public function getRunningJobs(){
    $jobs = array();
    // Normally here we would connect to the remote host
    // and check to see how many jobs are running, but for a
    // stand-alone SSH machine we can't do that because
    // we can't distringuish between the jobs we started
    // and those that may be running by virtue of the fact
    // we're using a real user account.  So, we'll just
    // have to count the entries for this resource
    // in the tripal_remote_job table.
    $sql = "
      SELECT TRJ.job_id
      FROM {tripal_remote_job} TRJ
        INNER JOIN {tripal_jobs} TJ on TJ.job_id = TRJ.job_id
      WHERE (TJ.status = 'Waiting' or TJ.status = 'Running') AND
        TRJ.resource_id = :resource_id
      ORDER BY TJ.submit_date ASC
    ";
    $results = db_query($sql, array(':resource_id' => $this->resource_id));
    while ($job_id = $results->fetchField()) {
      $job = new TripalRemoteJob();
      $job->load($job_id);
      $jobs[] = $job;
    }
    return $jobs;
  }

  /**
   * @see TripalRemoteResource::isJobRunning()
   */
  public function isJobRunning(TripalRemoteJob $job) {
      $hostname = $this->getHostname();
      $ssh_port = $this->getSSHPort();
      $username = $this->getUsername();
      $ssh_key_location = $this->getSSHKeyLocation();
      $ssh_key_dirOLD = variable_get('tripal_remote_job_ssh_key_dir', $_SERVER['HOME'] . ' /.ssh');
      
      //Get the remote PID of the job
      $job_id = $job->getJobID();
      echo "this job's ID: ". $job_id;
      echo "\n";
      
      //Assemble the command to get the PID (may change depending on environment)
      $ssh_conn = "ssh -p $ssh_port -i $ssh_key_location $username" . '@' . $hostname;
      
      $remote_job_dir = $this->getJobWorkingDir($job);
      
      echo "Checking for PID in remote file ".$remote_job_dir."/PID";
      echo "\n";
      
      /*
       * Get the Remote PID
       */
      $getPIDcmd = "$ssh_conn \"cat $remote_job_dir/PID\"";
      $getPIDoutput = array();
      $getPIDreturn = '';
      exec($getPIDcmd, $getPIDoutput, $getPIDreturn);
      $remotePID = $getPIDoutput[0];
      
      // Debug missing PID
      echo 'Process ID foudn: '.$getPIDoutput[0];
      /*
       * Check if the PID is still active
       */
      $getStatusCmd = "$ssh_conn \"ps -p $remotePID\"";
      $getStatusOutput = array();
      $getStatusReturn = '';
      exec($getStatusCmd, $getStatusOutput, $getStatusReturn);
      
      /* Debug
      echo "Line 1 of output: ".$getStatusOutput[0];
      echo "\n";
      echo "Line 2 of output: ".$getStatusOutput[1];
      echo "\n";
      echo "Length of line 2: ".strlen($getStatusOutput[1]);
      echo "\n";
      */
      
      if (strlen($getStatusOutput[1]) == 0)
      {
          echo "PID no longer active, returning FALSE";
          echo "\n";
          return FALSE;
      }
      else
      {
          echo "PID still active, returning TRUE";
          echo "\n";
          return TRUE;
      }
  }
  
  /**
   * Deletes the resource submit.
   *
   * @return boolean TURE|FALSE
   */
  public function delete() {

    // Delete an entry in resource_submit table.
    $transaction = db_transaction();
    try {
      parent::delete();
      db_delete('tripal_remote_ssh')
        ->condition('resource_id', $this->resource_id, '=')
        ->condition('hostname', $this->hostname, '=')
        ->execute();
    }
    catch (Exception $e) {
      $transaction->rollback();
      drupal_set_message('Error deleteing resource submit');
      watchdog('tripal_remote_job', $e->getMessage(), array(), WATCHDOG_ERROR);
      return FALSE;
    }
    return TRUE;
  }

  /**
   * @see TripalRemoteResource::removeWorkingDir()
   */
  public function removeWorkingDir(TripalRemoteJob $job) {

    parent::removeWorkingDir($job);

    // Get the SSH connection information.
    $hostname = $this->getHostname();
    $ssh_port = $this->getSSHPort();
    $username = $this->getUsername();
    $ssh_key_location = $this->getSSHKeyLocation();
    $ssh_conn = "ssh -p $ssh_port -i $ssh_key_location $username" . '@' . $hostname;
    $remote_job_dir = $this->getJobWorkingDir($job);

    // Command to delete the job diretory.
    //$cmd = "$ssh_conn rm -rf $remote_job_dir";
    //$job->logMessage('Cleanup Command: ' . $cmd);
    //exec($cmd);
    $job->logMessage('Skipping cleanup for TripalRemoteSSH.inc debugging');
  }

  /**
   * @see TripalRemoteResource::createWorkingDir()
   */
  public function createWorkingDir(TripalRemoteJob $job) {

    parent::createWorkingDir($job);

    // Get the SSH connection information.
    $hostname = $this->getHostname();
    $ssh_port = $this->getSSHPort();
    $username = $this->getUsername();
    $ssh_key_location = $this->getSSHKeyLocation();
    $ssh_conn = "ssh -p $ssh_port -i $ssh_key_location $username" . '@' . $hostname;
    $remote_job_dir = self::getJobWorkingDir($job);

    // Command to create the job diretory.
    $cmd = "$ssh_conn mkdir -p $remote_job_dir";
    $job->logMessage('Creation Command: ' . $cmd);
    exec($cmd);
  }

  /**
   * @see TripalRemoteResource::run()
   */
  public function run(TripalRemoteJob $job) {
    $job->logMessage("Running job '" . $job->getJobName() . "' [id:" . $job->getJobID() . "]");

    $this->prepareResultsDir($job);
    $this->createWorkingDir($job);
    $this->sendFiles($job);

    $hostname = $this->getHostname();
    $ssh_port = $this->getSSHPort();
    $username = $this->getUsername();
    $ssh_key_location = $this->getSSHKeyLocation();
    $ssh_conn = "ssh -p $ssh_port -i $ssh_key_location $username" . '@' . $hostname;
    $remote_job_dir = $this->getJobWorkingDir($job);
    $script_file = self::getJobRunScript($job);
    //$cmd = "$ssh_conn \"cd " . self::getJobWorkingDir($job) . "; sh $script_file > STDOUT.txt 2> STDERR.txt &\"";
    $cmd = "$ssh_conn \"cd " . self::getJobWorkingDir($job) . "; sh tripal_remote_job_" . $job->getJobID() . ".sh > STDOUT.txt 2>STDERR.txt\"";
    $output = array();
    $return_var = '';
    $job->logMessage('Execution Command: ' . $cmd);
    exec($cmd, $output, $return_var);
  }

  /**
   * @see TripalRemoteResource::getJobResults()
   */
  public function getJobResults(TripalRemoteJob $job) {
    $this->pullFiles($job);
  }

  /**
   * @see TripalRemoteResource::cleanJob()
   */
  public function cleanJob(TripalRemoteJob $job) {
    $this->removeWorkingDir($job);
  }

  /**
   * @see TripalRemoteResource::sendFiles()
   */
  public function sendFiles(TripalRemoteJob $job) {
    parent::sendFiles($job);

    // Get the SSH connection information.
    $hostname = $this->getHostname();
    $ssh_port = $this->getSSHPort();
    $username = $this->getUsername();
    $ssh_key_location = $this->getSSHKeyLocation();
    $conn_str = $username . '@' . $hostname . ':';
    $remote_job_dir = $this->getJobWorkingDir($job);
    $rsync = variable_get('tripal_remote_job_rsync_binary', '/usr/bin/rsync');

    // Copy the files to the remote server for execution.
    $files = $job->getFiles();
    foreach ($files as $filepath) {
      $job->logMessage("Sending file '$filepath'...");
      $output = array();
      $return_var = '';
      $cmd = "$rsync -rtz -e 'ssh -p $ssh_port -i $ssh_key_location' --exclude 'run.lock' $filepath $conn_str$remote_job_dir";
      $job->logMessage('Send Command: ' . $cmd);
      exec($cmd, $output, $return_var);
    }
  }

  /**
   * @see TripalRemoteResource::pullFiles()
   */
  public function pullFiles(TripalRemoteJob $job) {
    parent::pullFiles($job);

    // Get the connection information from TRJobResource.
    $resource = $job->getResource();
    $hostname = $resource->getHostname();
    $ssh_port = $resource->getSSHPort();
    $username = $resource->getUsername();
    $ssh_key_location = $this->getSSHKeyLocation();
    $conn_str = $username . '@' . $hostname . ':';
    $remote_job_dir = $this->getJobWorkingDir($job);
    $local_working_dir = $this->getResultsDir();
    $rsync = variable_get('tripal_remote_job_rsync_binary', '/usr/bin/rsync');

    // Perform rsync only if the remote directory exists.
    $cmd = "$rsync -rtz -e 'ssh -p $ssh_port -i $ssh_key_location' $conn_str$remote_job_dir $local_working_dir";
    $job->logMessage('Pull Command: ' . $cmd);
    $output = array();
    $return_var = '';
    exec($cmd, $output, $return_var);
  }

  /**
   * Retrieves the hostname.
   *
   * @return string
   */
  public function getHostname() {
    return $this->hostname;
  }

  /**
   * Updates the hostname.
   *
   * @param string $hostname
   */
  public function setHostname($hostname) {
    $this->hostname = $hostname;
  }

  /**
   * Retrieves the port number of SSH.
   *
   * @return string
   */
  public function getSSHPort() {
    return $this->ssh_port;
  }

  /**
   * Updates the port number of SSH.
   *
   * @param string $ssh_port
   */
  public function setSSHPort($ssh_port) {
    $this->ssh_port = $ssh_port;
  }

  /**
   * Retrieves the username.
   *
   * @return string
   */
  public function getUsername() {
    return $this->username;
  }

  /**
   * Updates the username.
   *
   * @param string $username
   */
  public function setUsername($username) {
    $this->username = $username;
  }
  
   /**
   * Retrieves the SSH Key location.
   * 
   * @return string
   */
  public function getSSHKeyLocation() {
    return $this->ssh_key_location;
  }
  
  /**
   * Updates the SSH Key location
   */
  public function setSSHKeyLocation($ssh_key_location) {
    $this->$ssh_key_location = $ssh_key_location;
  }

}