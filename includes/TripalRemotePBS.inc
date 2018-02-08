<?php

/**
 * The declaration of TRJobResourcePBS
 *
 */
class TripalRemotePBS extends TripalRemoteSSH {

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
   * Generates a TRJobResourcePBS object using a provided ID.
   *
   * @param $resource_id
   *
   * @return TRJobResourcePBS|NULL
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

    // define here...
  }

  /**
   * @see TRJobResource::killJob()
   */
  public function killJob(TripalRemoteJob $job) {

    // define here...
  }

  /**
   * @see TRJobResource::runCommand()
   */
  public function runCommand(TRJobJobExec $job, $command, $index = 0, $options = array()) {

    // Set default options.
    $outfile = $options['outfile'] ? $options['outfile'] : '';
    $no_stdout = $options['no_stdout'] ? $options['no_stdout'] : FALSE;

    // define here...
  }
}