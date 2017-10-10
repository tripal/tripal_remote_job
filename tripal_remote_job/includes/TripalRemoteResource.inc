<?php
/**
 * @file
 *
 * This file provides functions for managing resources where system commands
 * can be submitted for execution.
 */

/**
 * The declaration of TripalRemoteResource class.
 *
 */
class TripalRemoteResource {

  // TRJobResource data members.
  protected $resource_id;
  protected $name;
  protected $type;
  protected $max_slots;
  protected $rank;
  protected $enabled;
  protected $working_directory;
  protected $description;


  /**
   * Implements the class constructor.
   *
   * @param $details
   */
  public function __construct($details = array()) {
    $this->resource_id  = array_key_exists('resource_id', $details) ? $details['resource_id'] : '';
    $this->name         = array_key_exists('name', $details)        ? $details['name']        : '';
    $this->type         = array_key_exists('type', $details)        ? $details['type']        : '';
    $this->max_slots    = array_key_exists('max_slots', $details)   ? $details['max_slots']   : 1;
    $this->rank         = array_key_exists('rank', $details)        ? $details['rank']        : NULL;
    $this->enabled      = array_key_exists('enabled', $details)     ? $details['enabled']     : 0;
    $this->description  = array_key_exists('description', $details) ? $details['description'] : '';
    $this->working_directory  = array_key_exists('working_directory', $details)   ? $details['working_directory']   : '';
  }

  /**
   * Implements the class destructor.
   */
  public function __destruct() {

  }

  /**
   * Create TRJobResource object by provided ID.
   *
   * @param $resource_id
   *
   * @return TRJobResource|NULL
   */
  public static function byID($resource_id) {
    $sql = "SELECT * FROM {tripal_remote_resource} WHERE resource_id = :resource_id";
    $resource = db_query($sql, array(':resource_id' => $resource_id))->fetch(PDO::FETCH_ASSOC);
    if ($resource) {
      return new self($resource);
    }
    else {
      return NULL;
    }
  }

  /**
   * Returns all the resources.
   *
   * @return array
   *  A list of TRJobResource objects.
   */
  public static function getResources() {
    $sql = "SELECT resource_id FROM {tripal_remote_resource} ORDER BY rank";
    $results = db_query($sql);
    $resources = array();
    while ($resource_id = $results->fetchField()) {
      $resources[] = self::getResource($resource_id);
    }
    return $resources;
  }

  /**
   * Returns the TRJobResource object.
   *
   * @param integer $resource_id
   *
   * @return TRJobResource|NULL
   */
  public static function getResource($resource_id) {
    $resource = NULL;

    module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteResource');
    module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteSSH');

    $res = TripalRemoteResource::byID($resource_id);
    if ($res) {
      // Get the resource type.
      $resource_type = $res->getType();

      // Get the resource.
      switch ($resource_type) {
        case 'alone' :
          $resource = TripalRemoteSSH::byID($resource_id);
          break;
        case 'pbs' :
          module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemotePBS');
          $resource = TripalRemotePBS::byID($resource_id);
          break;
        case 'ge' :
          module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteGE');
          $resource = TripalRemoteGE::byID($resource_id);
          break;
        case 'slurm' :
          module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteSLURM');
          $resource = TripalRemoteSLURM::byID($resource_id);
          break;
        default :
          throw new Exception("Unsupported computational resource type: $resource_type.");
          break;
      }
    }
    return $resource;
  }

  /**
   * Return next rank.
   *
   * @return integer
   */
  public static function getNextRank() {
    $sql = "SELECT MAX(rank) FROM {tripal_remote_resource}";
    return intval(db_query($sql)->fetchField()) + 1;
  }

  /**
   * Returns the number of available slots.
   *
   * @return
   *   The number of free "slots" for job execution.
   */
  public function getFreeSlots() {

    // Count number of free slots.
    $jobs = $this->getRunningJobs();
    $running = count($jobs);
    $num_slots = $this->max_slots - $running;
    if ($num_slots < 0) {
      $num_slots = 0;
    }
    return $num_slots;
  }

  /**
   * Check if the job is running.
   *
   * @param TripalRemoteJob $job
   *
   * @return boolean TRUE|FALSE
   */
  public function isJobRunning(TripalRemoteJob $job) {
    // To be overridden by Child class
    return FALSE;
  }

  /**
   * Kill the running job.
   *
   * @param TripalRemoteJob $job
   *
   * @return boolean TRUE|FALSE
   */
  public function killJob(TripalRemoteJob $job) {
    // To be overridden by Child class
    return TRUE;
  }

  /**
   * Run the command line.
   *
   * @param TripalRemoteJob $job
   */
  public function run(TripalRemoteJob $job) {
    $job->logMessage("Running job '" . $job->getJobName() . "' [id:" . $job->getJobID() . "]");

    self::prepareResultsDir($job);
    self::createWorkingDir($job);
    self::sendFiles($job);

    // To be overridden by Child class.
  }

  /**
   * Retrieves the results files and places them in the local results directory.
   *
   * @param TripalRemoteJob $job
   */
  public function getJobResults(TripalRemoteJob $job) {
    self::pullFiles($job);
  }

  /**
   * Cleans up any unneeded items on the remote system.
   *
   * @param TripalRemoteJob $job
   */
  public function cleanJob(TripalRemoteJob $job) {
    self::removeWorkingDir($job);
  }

  /**
   * Copying files to the remote location.
   *
   * @param TripalRemoteJob $job
   */
  protected function sendFiles(TripalRemoteJob $job) {
    if (count($job->getFiles()) > 0) {
      $job->logMessage('Sending files to remote resource...');
    }

    // To be overridden by child class.
  }

  /**
   * Copying files from the remote to the loccal location.
   *
   * @param TripalRemoteJob $job
   */
  protected function pullFiles(TripalRemoteJob $job) {
    if (count($job->getFiles()) > 0) {
      $job->logMessage('Retrieving results from remote resource...');
    }

    // To be overridden by child class.
  }

  /**
   * Removes the remote working directory
   *
   * @param TripalRemoteJob $job
   */
  protected function removeWorkingDir(TripalRemoteJob $job) {
    $job->logMessage('Removing remote working directory...');
    // To be overridden by child class.
  }

  /**
   * Creates the remote working directory.
   *
   * @param TripalRemoteJob $job
   */
  protected function createWorkingDir(TripalRemoteJob $job) {
     $job->logMessage('Creating remote working directory...');
     // To be overridden by child class.
  }

  /**
   * Gets the name of the script containing the execution commands.
   *
   * @param TripalRemoteJob $job
   */
  protected function getJobRunScript(TripalRemoteJob $job) {
    $local_working_dir = $this->getJobResultsDir($job);
    return $local_working_dir . '/tripal_remote_job_' . $job->getJobID() . '.sh';
  }

  /**
   * Creates the local working directory.
   *
   * @param TripalRemoteJob $job
   */
  protected function prepareResultsDir(TripalRemoteJob $job) {
    $local_working_dir = $this->getJobResultsDir($job);

    // Make sure we have a local directory to put results into.
    $job->logMessage("Trying to create ".$local_working_dir);
    if (!file_exists($local_working_dir)) {
      mkdir($local_working_dir, 0777, TRUE);
      $job->logMessage("Job diretory is created: $local_working_dir");
    }
    else
        $job->logMessage("Job directory already exists: $local_working_dir");

    // Now add the run script to the directory
    $script_file = self::getJobRunScript($job);
    $contents = $job->getCommand();
    //pumpkin remove $contents = 'uptime';
    $fh = fopen($script_file, 'w');
    fwrite($fh, $contents);
    fclose($fh);

    // Add the script to the files array for copying over.
    $files = $job->getFiles();
    $files[] = $script_file;
    $job->setFiles($files);
  }

  /**
   * Adds a new resource properties.
   *
   * @return boolean TRUE|FALSE
   */
  public function insert() {

    // Insert the resource from tripal_remote_resource.
    try {
      $this->resource_id = db_insert('tripal_remote_resource')
        ->fields(array(
          'name'              => $this->name,
          'type'              => $this->type,
          'max_slots'         => $this->max_slots,
          'rank'              => $this->rank,
          'enabled'           => $this->enabled,
          'working_directory' => $this->working_directory,
          'description'       => $this->description
        ))
        ->execute();
    }
    catch (Exception $e) {
      watchdog('tripal_remote_job', $e->getMessage(), array(), WATCHDOG_ERROR);
      return FALSE;
    }
    return TRUE;
  }

  /**
   * Updates resorce properties.
   *
   * @return boolean TRUE|FALSE
   */
  public function update() {

    // Update the resource from tripal_remote_resource.
    try {
      db_update('tripal_remote_resource')->fields(array(
        'name'              => $this->name,
        'type'              => $this->type,
        'max_slots'         => $this->max_slots,
        'rank'              => $this->rank,
        'enabled'           => $this->enabled,
        'working_directory' => $this->working_directory,
        'description'       => $this->description
      ))
      ->condition('resource_id', $this->resource_id, '=')
      ->execute();
    }
    catch (Exception $e) {
      watchdog('tripal_remote_job', $e->getMessage(), array(), WATCHDOG_ERROR);
      return FALSE;
    }
    return TRUE;
  }

  /**
   * Delete the resource.
   *
   * @return boolean TRUE|FALSE
   */
  public function delete() {

    // Remove the resource from tripal_remote_resource.
    try {
      db_delete('tripal_remote_resource')
        ->condition('resource_id', $this->resource_id, '=')
        ->execute();
    }
    catch (Exception $e) {
      watchdog('tripal_remote_job', $e->getMessage(), array(), WATCHDOG_ERROR);
      $transaction->rollback();
      return FALSE;
    }
    return TRUE;
  }

  /**
   * Returns the list of running jobs.
   *
   * @return array
   */
  public function getRunningJobs() {
    $jobs = array();
    // TODO: impelement this function.
    return $jobs;
  }

  /**
   * Disable the resource.
   */
  public function disable() {
    $this->enabled = 0;
  }

  /**
   * Enable the resource.
   */
  public function enable() {
    $this->enabled = 1;
  }

  /**
   * Return if this resource is enbled.
   *
   * @return integer
   */
  public function isEnabled() {
    return $this->enabled;
  }

  /**
   * Retrieves the max slot.
   *
   * @return integer
   */
  public function getMaxSlots() {
    return $this->max_slots;
  }

  /**
   * Updates the max slots.
   *
   * @param integer $slots
   */
  public function setMaxSlots($slots) {
    $this->max_slots = $slots;
  }

  /**
   * Retrieves the resource ID.
   *
   * @return integer
   */
  public function getResourceID() {
    return $this->resource_id;
  }

  /**
   * Updates the resource ID.
   *
   * @return integer $resource_id
   */
  public function setResourceID($resource_id) {
    $this->resource_id = $resource_id;
  }

  /**
   * Retrieves the type of the resource.
   *
   * @return string
   */
  public function getType() {
    return $this->type;
  }

  /**
   * Updates the type of the resource.
   *
   * @return string
   */
  public function setType($type) {
    // Make sure the type is correct
    if ($type == "alone" or $type == "slurm" or $type == "pbs" or $type == "ge") {
      $this->type = $type;
    }
    // TODO: how to setters return errors.
  }

  /**
   * Retrieves the rank of the resource.
   *
   * @return Ambigous <string, unknown>
   */
  public function getRank() {
    return $this->rank;
  }
  /**
   * Updates the rank of the resource.
   *
   * @return integer
   */
  public function setRank($rank) {
    $this->rank = $rank;
  }

  /**
   * Retrieves the working directory on the remote server.
   *
   * @return string
   */
  public function getWorkingDir() {
    return $this->working_directory;
  }

  /**
   * Retrieves the working directory specific for a job on the remote server.
   *
   * @param TripalRemoteJob $job
   *
   * @return
   *   The full path to the job's remote working directory.
   */
  public function getJobWorkingDir(TripalRemoteJob $job) {
    return $this->working_directory . '/tripal_remote_job/' . $job->getJobID();
  }

  /**
   * Retrieves the path to the directory where results are found for the job.
   *
   * @param TripalRemoteJob $job
   *
   * @return
   *   The full path to the results directory.
   */
  public function getJobResultsDir(TripalRemoteJob $job){
      return $this->getResultsDir() . '/' . $job->getJobID();
  }

  /**
   * Retrieves the path to the directory where all job results are stored.
   *
   * @return string
   */
  public function getResultsDir(){
    return drupal_realpath('public://') . '/tripal/jobs';
  }

  /**
   * Updates the working directory.
   *
   * @param string $dir
   */
  public function setWorkingDir($dir) {
    $this->working_directory = $dir;
  }

  /**
   * Retrieves the description of the resource.
   *
   * @return string
   */
  public function getDescription() {
    return $this->description;
  }

  /**
   * Updates the description of the resource.
   *
   * @param string $name
   */
  public function setDescription($description) {
    $this->description = $description;
  }

  /**
   * Retrieves the name of the resource.
   *
   * @return string
   */
  public function getName() {
    return $this->name;
  }

  /**
   * Updates the name of the resource.
   *
   * @param string $name
   */
  public function setName($name) {
    $this->name = $name;
  }
}

