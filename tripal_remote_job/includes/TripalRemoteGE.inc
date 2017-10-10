<?php


/**
 * The declaration of TRJobResourceGE class.
 *
 */
class TripalRemoteGE extends TripalRemoteSSH {

  /**
   * Implements the class constructor.
   *
   * @param $details
   */
  public function __construct($details = array()) {
    parent::__construct($details);
  }

  /**
   * Implements the class destructor.
   */
  public function __destruct() {
    parent::__destruct();
  }

  /**
   * Generates a TRJobResourceGE object using a provided ID.
   *
   * @param $resource_id
   *
   * @return TRJobResourceGE|NULL
   */
  public static function byID($resource_id) {

    // Get the resource info.
    $sql = "SELECT * FROM {tripal_remote_resource} WHERE resource_id = :resource_id";
    $resource = db_query($sql, array(':resource_id' => $resource_id))->fetch(PDO::FETCH_ASSOC);

    // Add connection info.
    $sql = "SELECT * FROM {tripal_remote_ssh} WHERE resource_id = :resource_id";
    $resource_submit = db_query($sql, array(':resource_id' => $resource_id))->fetch(PDO::FETCH_ASSOC);
    $resource['hostname'] = $resource_submit['hostname'];
    $resource['ssh_port'] = $resource_submit['ssh_port'];
    $resource['username'] = $resource_submit['username'];

    // Return the instance of this class.
    if ($resource) {
      return new self($resource);
    }
    else {
      return NULL;
    }
  }

  /**
   * @see TRJobResource::isJobRunning()
   */
  public function isJobRunning(TripalRemoteJob $job) {

    // Get JobExec ID. If it is empty return FALSE.
    $exec_id = $this->getJobExecID($job);
    if (!$exec_id) {
      return FALSE;
    }

    // Search the status of the submitted job in qstat table.
    // Make sure to add -u option.
    $username = $this->getUsername();
    $output = array();
    $return_var = '';
    exec("qstat -u $username", $output, $return_var);
    foreach ($output as $line) {
      $line = trim($line);
      $cols = preg_split('/\s+/', $line);
      if ($cols[0] == $exec_id) {
        return TRUE;
      }
    }
    return FALSE;
  }

  /**
   * @see TRJobResource::killJob()
   */
  public function killJob(TripalRemoteJob $job) {

    // Kill the job if it is running.
    if ($this->isJobRunning($job)) {

      // Get the PID for this job.
      $pid = $this->getJobExecID($job);
      exec("qdel -j $pid", $output, $status);
      return $status;
    }
    return TRUE;
  }

  /**
   * @see TRJobResource::runCommand()
   */
  public function run(TripalRemoteJob $job) {

    $command = $job->getCommand();

    // Prepare some commonly used variables.
    $project    = $job->getProject();
    $project_id = $project->getProjectID();
    $job_id     = $job->getJobID();
    $job_dir    = $job->getWorkingDir();
    $prefix     = $job->getJobFilePrefix();

    // If the user wants to redirect into a specific outfile then do so,
    // but, create a blank STDOUT file so we don't break logfile parsing.
    $full_command = '';
    if ($outfile) {
      $full_command = "$command > $outfile 2> $outfile.stderr ";
    }
    else {
      $full_command = $command;
    }

    // Create the submission file
    $script  = "#!/bin/bash\n";
    $script .= "#\n";
    $script .= "#\$ -N GS_$job_id\n";
    $script .= "#\$ -o $stdout_f\n";
    $script .= "#\$ -e $stderr_f\n";
    $script .= $full_command;

    // Create the submission file in the local directory and then sync it so that
    // it can be launced on the cluster head node.
    $qsub_f = "$prefix.$index.qsub";
    file_put_contents($job->getLocalWorkingDir() . '/' . $qsub_f, $script);
    $this->sendFiles($job);

    // Get the SSH connection information.
    $hostname     = $this->getHostname();
    $ssh_port     = $this->getSSHPort();
    $username     = $this->getUsername();
    $ssh_key_dir = variable_get('tripal_remote_job_ssh_key_dir', $_SERVER['HOME'] . ' /.ssh');
    $ssh_conn     = "ssh -p $ssh_port -i $ssh_key_dir/id_rsa $username" . '@' . $hostname;

    // Execute the command.
    $cmd = "$ssh_conn qsub " . $job->getWorkingDir() . '/' . $qsub_f;
    exec($cmd, $output, $return_var);

    // Check the return status of the submitted job.
    switch ($return_var) {

      // The job was successfully submitted.
      case 0:
        // Get the job ID
        $job_id = $output[0];
        $job_id = preg_replace('/Your job (\d+) \(.*?\) has been submitted/', '$1', $job_id);
        $this->addJobExecID($job, $job_id);
        return TRUE;
        break;
      // The job was not submitted because the limit was reached
      case 25:
        return FALSE;
        break;
      // Some other failure led to the job not being submitted.
      default:
        return FALSE;
        break;
    }
  }
}
