<?php
/**
 * The declaration of TripalRemoteScheduler Class.
 *
 */
class TripalRemoteScheduler {

  // The list of jobs currently in the queue either waiting or running.
  private $queue = array();

  // The list of resources available for running jobs.
  private $resources = array();


  /**
   * Implements the class constructor.
   */
  public function __construct() {
    // Get the list of available resources, and rank them in order of
    // prefered use. Local is always available.
    module_load_include('inc', 'tripal_remote_job', 'includes/TripalRemoteResource');
    $this->resources = TripalRemoteResource::getResources();

    // Popuplate the job queue.
    $this->refreshQueue();
  }

  /**
   * Implements the class destructor.
   */
  public function __destruct() {
  }


  /**
   * Refreshes the job queue of the scheduler.
   */
  protected function refreshQueue() {

    // First, Clean up any completed, cancelled or errored jobs:
    $sql = "
      DELETE FROM {tripal_remote_job} WHERE job_id IN (
        SELECT TRJ.job_id
        FROM {tripal_remote_job} TRJ
          INNER JOIN {tripal_jobs} TJ on TJ.job_id = TRJ.job_id
        WHERE NOT TJ.status = 'Waiting' AND NOT TJ.status = 'Running'
        ORDER BY TJ.submit_date ASC)
    ";
    $results = db_query($sql);

    // Second, get the jobs that are left.
    $sql = "
      SELECT TRJ.job_id, TRJ.resource_id
      FROM {tripal_remote_job} TRJ
        INNER JOIN {tripal_jobs} TJ on TJ.job_id = TRJ.job_id
      ORDER BY TJ.submit_date ASC
    ";
    $results = db_query($sql);
    while ($record = $results->fetchObject()) {
      $job = new TripalRemoteJob();
      $job->load($record->job_id);
      $this->queue[] = $job;
    }
  }

  /**
   * Execute a remote job.
   */
  public function launchJob(TripalRemoteJob $job) {
    $hold = array();
    $num_user_jobs = 0;
    $max_jobs = 7;
    $done = FALSE;
    $wait_sleep = 60;
    $this_uid = $job->getUID();
    $user = user_load($this_uid);

    // Update the job queue.
    $this->refreshQueue();


    // Loop continously until we can schedule the job on a resource.
    while (!$done) {

      // Determine how manu jobs the user is currently running
      foreach ($this->queue as $qjob) {
        if ($qjob->getUID() == $this_uid) {
          $num_user_jobs++;
        }
      }
      if ($num_user_jobs > $max_jobs) {
        $job->logMessage($user->name . " is running more than $max_jobs jobs. Waiting for jobs to finish");
        sleep($wait_sleep);
        continue;
      }

      // Find a remote resource for this job.
      $this->assignJobResource($job);
      $resource = $job->getResource();
      if (!$resource) {
        throw new Exception("Could not assign a remote execution resource to the job.");
      }

      // Start the job!
      if ($job->getSendEmail() == TRUE) {
        $this->sendStartEmail($job);
      }
      $job->setProgress(0);
      $resource->run($job);

      $done = TRUE;
    }
  }

  /**
   * Checks the status of the running job. Notifies the module 
   * about completion if supported by the module
   *
   * @param TripalRemoteJob $job
   *
   * @return
   *   A status string indicating: 'running', or 'completed'.
   *   If an error occurs then an exception is thrown.
   *
   * @throws Exception
   */
  public function checkJobStatus(TripalRemoteJob $job) {
    $resource = $job->getResource();
    if ($resource->isJobRunning($job)) {
      $job->logMessage('Job is still running....\n');
      return 'running';
    }
    $job->setProgress(100);
    /**
     * Check if the original module has notify functionality that we can call
     */    
    // Does the module still exist (most likely but you never know)
    $origin_module = $job->getModuleName();
    $job_id = $job->getJobID();
    $notify_module = TRUE;
    if(!module_exists($origin_module))
    {
        $notify_module = FALSE;
    }
    $notify_function = $origin_module.'_trj_notify';
    
    // Does the module have a mail function?
    if(!function_exists($notify_function))
    {
        $notify_module = FALSE;
    }
    if($notify_module)
    {
        echo "--__--__--    attempting to notify the original module about completion";
        echo "Trying to call " . $notify_function;
        echo "";
        $notify_function($job_id,'completed');
    }
    return 'completed';    
  }

  /**
   * Get Job results.
   * @param TripalRemoteJob $job
   */
  public function getResults(TripalRemoteJob $job) {
    $resource = $job->getResource();
    $resource->getJobResults($job);
  }

  /**
   * Cleans up the job.
   *
   * @param TripalRemoteJob $job
   */
  public function cleanJob(TripalRemoteJob $job) {
    $resource = $job->getResource();
    $resource->cleanJob($job);
    if ($job->getSendEmail() == TRUE) {
      $this->sendCompleteEmail($job);
    }
  }

  /**
   * Send the notification email (start).
   *
   * @param TripalRemoteJob $job
   */
  protected function sendStartEmail(TripalRemoteJob $job) {

    // Get the user for this job.
    $uid = $job->getUID();
    $user = user_load($uid);
    // Try to get the module name, set $use_module_mail based on success
    $use_module_mail = TRUE;
    $origin_module = $job->getModuleName(); //? $use_module_mail = TRUE : $use_module_mail = FALSE;
    
    /**
     * Check if the original module has mail functionality that we can call
     */    
    // Does the module still exist (most likely but you never know)
    if(!module_exists($origin_module))
    {
        $use_module_mail = FALSE;
    }
    $custom_mail_function = $origin_module.'_mail';
    
    // Does the module have a mail function?
    if(!function_exists($custom_mail_function))
    {
        $use_module_mail = FALSE;
    }
    
    /*
     * Prepare the drupal_mail parameters
     */
    
    // Use the specified module or fallback to Tripal Remote Job
    $use_module_mail ? $module = $origin_module : $module = 'tripal_remote_job';
    $key = 'job-started';
    
    // Specify 'to' and 'from' addresses.
    $to = $user->mail;
    $from = variable_get('site_mail', 'admin@localhost.localdomain');

     /*"params" loads in additional context for email content completion in
     hook_mail(). In this case, we want to pass in the values the user entered
     into the form, which include the message body in $form_values['message'].
      */
    $params = array(
        'job'     => $job,
        'user'    => $user,
        'origin_module' => $origin_module,
    );

     /*The language of the e-mail. This will one of three values:
     * user_preferred_language(): Used for sending mail to a particular website
       user, so that the mail appears in their preferred language.
     * global $language: Used when sending a mail back to the user currently
       viewing the site. This will send it in the language they're currently
       using.
     * language_default(): Used when sending mail to a pre-existing, 'neutral'
       address, such as the system e-mail address, or when you're unsure of the
       language preferences of the intended recipient.
    
     Since in our case, we are sending a message to a random e-mail address that
     is not necessarily tied to a user account, we will use the site's default
     language.
      * 
      */
    $language = language_default();

     /*Whether or not to automatically send the mail when drupal_mail() is
     called. This defaults to TRUE, and is normally what you want unless you
     need to do additional processing before drupal_mail_send() is called.
      */
    $send = TRUE;

     /*Send the mail, and check for success. Note that this does not guarantee
     message delivery; only that there were no PHP-related issues encountered
     while sending.
      */
    $result = drupal_mail($module, $key, $to, $language, $params, $from, $send);
    if ($result['result'] != TRUE) {
      watchdog('tripal_remote_job', t('There was a problem sending your message and it was not sent.'), array(), WATCHDOG_ERROR);
    }
    
    // Alert an admin that some module requested a job
    //$result = drupal_mail('tripal_remote_job','test',$params);
  }

  /**
   * Send the notification email (completion).
   *
   * @param TripalRemoteJob $job
   */
  protected function sendCompleteEmail(TripalRemoteJob $job) {
    // Exactly the same as sendStartEmail, except $key is indication of job status
    // Refer to that function for comments
    $uid = $job->getUID();
    $user = user_load($uid);
    $use_module_mail = TRUE;
    $origin_module = $job->getModuleName(); //? $use_module_mail = TRUE : $use_module_mail = FALSE;
    
    if(!module_exists($origin_module))
    {
        $use_module_mail = FALSE;
    }
    $custom_mail_function = $origin_module.'_mail';
    if(!function_exists($custom_mail_function))
    {
        $use_module_mail = FALSE;
    }
    
    $use_module_mail ? $module = $origin_module : $module = 'tripal_remote_job';
    $key = 'job-complete';
    
    $to = $user->mail;
    $from = variable_get('site_mail', 'admin@localhost.localdomain');

     $params = array(
        'job'     => $job,
        'user'    => $user,
        'origin_module' => $origin_module,
    );

     $language = language_default();

     $send = TRUE;

     $result = drupal_mail($module, $key, $to, $language, $params, $from, $send);
    if ($result['result'] != TRUE) {
      watchdog('tripal_remote_job', t('There was a problem sending your message and it was not sent.'), array(), WATCHDOG_ERROR);
    }
  }

  /**
   * Assigns a resource to a job.
   *
   * @param TripalJob $job
   *
   * @return TripalRemoteResource
   */
  protected function assignJobResource(TripalRemoteJob $job) {
    foreach ($this->resources as $resource) {
      if ($resource->isEnabled() and $resource->getFreeSlots() > 0) {
        db_insert('tripal_remote_job')
          ->fields(array(
            'job_id' => $job->getJobID(),
            'resource_id' => $resource->getResourceID(),
          ))
          ->execute();
        $job->setResource($resource);
        return TRUE;
      }
    }
    return FALSE;
  }
}
