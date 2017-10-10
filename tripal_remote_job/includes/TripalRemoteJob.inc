<?php

class TripalRemoteJob extends TripalJob {

  /**
   * The set of files used by this job
   */
  private $files = array();

  /**
   * The TripalRemoteResource (or child class) assigned to this job.
   */
  private $resource = NULL;

  /**
   * Indicates if the user should be notified by email when the job starts and
   * ends.
   */
  private $send_email = FALSE;

  /**
   * The command to execute on the remote host.
   */
  private $command = '';


  /**
   * @see TripalJob::load()
   */
  public function load($job_id) {
    parent::load($job_id);
    
    $this->command = $this->job->arguments[0];
    $this->files = $this->job->arguments[1];
    $this->send_email = $this->job->arguments[2];
    //print_r($this); For debugging contents of the job
    //Note that $this->job is an object of type TripalJob 
    //and not TripalRemoteJob
    
    $this->resource = NULL;
    $resource_id = db_select('tripal_remote_job', 'trj')
      ->fields('trj', array('resource_id'))
      ->condition('trj.job_id', $job_id)
      ->execute()
      ->fetchField();
    if ($resource_id) {
      module_load_include('inc', 'tripal_remote_job', 'includes/TripalRemoteResource');
      $this->resource = TripalRemoteResource::getResource($resource_id);
    }
  }


  /**
   * Sets the resource for this job.
   */
  public function setResource($resource) {
    $this->resource = $resource;
  }
  /**
   * Gets the resource assigned to the job.
   */
  public function getResource() {
    return $this->resource;
  }
  public function setFiles($files) {
    $this->files = $files;
  }
  public function getFiles(){
    return $this->files;
  }
  public function setCommand($command) {
    $this->command = $command;
  }
  public function getCommand() {
    return $this->command;
  }
  public function setSendEmail($send_email) {
    $this->send_email = $send_email;
  }
  public function getSendEmail() {
    return $this->send_email;
  }

}