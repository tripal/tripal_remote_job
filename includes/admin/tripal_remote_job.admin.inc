<?php


// $form['tripal_remote_job_rsync_binary'] = array(
//   '#title'       => t('The rsync Binary'),
//   '#type'        => t('textfield'),
//   '#description' => t("The rsync utility will be used for syncing files between
//         the local server and remote computational resources.  Please specify
//         the full path to the rsync binary."),
//   '#required'    => TRUE,
//   '#default_value' => variable_get('tripal_remote_job_rsync_binary', '/usr/bin/rsync'),
// );


/**
 * Admin resources page
 *
 */
function tripal_remote_job_admin_resources_page() {

  module_load_include('inc', 'tripal_remote_job', 'includes/TripalRemoteResource');

  // Set the breadcrumb.
  $breadcrumb = array();
  $breadcrumb[] = l('Home', '<front>');
  $breadcrumb[] = l('Administration', 'admin');
  $breadcrumb[] = l('Tripal', 'admin/tripal');
  $breadcrumb[] = l('Extensions', 'admin/tripal/extension');
  $breadcrumb[] = l('Remote Computational Resources', 'admin/tripal/extension/remote-resources');
  drupal_set_breadcrumb($breadcrumb);

  // Set the header and rows for the job queue table.
  $headers = array('', 'Resource Name', 'Resource Type', 'Rank', 'Enabled', 'Slots', 'Description', 'Actions');
  $rows = array();

  // Get all resources.
  $resources = TripalRemoteResource::getResources();

  $num_recs = 1; // 
  foreach ($resources as $resource) {
    $resource_id    = $resource->getResourceID();
    $resource_type  = $resource->getType();

    $edit_link = l('edit', 'admin/tripal/extension/remote-resources/edit/' . $resource_id);
    $delete_link = ($resource_type == 'local') ? '' : l('delete', 'admin/tripal/extension/remote-resources/delete/' . $resource_id);
    $enabled = ($resource->isEnabled()) ? 'YES' : 'NO';

    $label = '';
    switch ($resource_type) {
      case 'alone':
        $label = 'Remote stand-alone computational server';
        break;
      case 'ge':
        $label = 'GridEngine cluster';
        break;
      case 'pbs':
        $label = 'PBS compatible cluster';
        break;
      case 'slurm':
        $label = 'SLURM cluster';
        break;
    }
    $rows[] = array(
      $num_recs,
      $resource->getName(),
      $label,
      $resource->getRank(),
      $enabled,
      $resource->getMaxSlots(),
      $resource->getDescription(),
      $edit_link . "&nbsp;&nbsp;" . $delete_link
    );
    $num_recs++;
  }

  $contents = array();

  $contents['instructions'] = array(
    '#type' => 'markup',
    '#markup' => '<p>' . t('The following computational resources are avilable for
        this site.  Jobs will be scheduled on resources in order of their rank.
        If a resource is fully subscribed then new jobs will be added to the
        next available resource with available slots.') . '</p>',
  );

  // Theme and return the table.
  $table_vars = array(
    'header'      => $headers,
    'rows'        => $rows,
    'attributes'  => array(),
    'sticky'      => TRUE,
    'colgroups'   => array(),
    'empty'       => t('There are no resources in the queue.'),
  );
  $contents['resource_list'] = array(
    '#type' => 'markup',
    '#markup' => theme('table', $table_vars)
  );
  return $contents;
}

/**
 * Page for editing the existing resource
 *
 */
function tripal_remote_job_admin_resource_form($form, &$form_state, $resource_id = NULL) {

  // Set the breadcrumb.
  $breadcrumb = array();
  $breadcrumb[] = l('Home', '<front>');
  $breadcrumb[] = l('Administration', 'admin');
  $breadcrumb[] = l('Tripal', 'admin/tripal');
  $breadcrumb[] = l('Extensions', 'admin/tripal/extension');
  $breadcrumb[] = l('Remote Computational Resources', 'admin/tripal/extension/remote-resources');
  drupal_set_breadcrumb($breadcrumb);

  module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteResource');
  module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteSSH');

  // Get TripalRemoteResource object.
  $resource           = NULL;
  $resource_libraries = NULL;
  $resource_tools     = NULL;
  if ($resource_id) {

    $res = TripalRemoteResource::byID($resource_id);
    if ($res->getType() == 'alone') {
      $resource = TripalRemoteSSH::byID($resource_id);
    }
    else if ($res->getType() == 'pbs') {
      module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemotePBS');
      $resource = TripalRemotePBS::byID($resource_id);
    }
    else if ($res->getType() == 'ge') {
      module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteGE');
      $resource = TripalRemoteGE::byID($resource_id);
    }
    else if ($res->getType() == 'slurm') {
      module_load_include('inc', 'tripal_remote_job', '/includes/TripalRemoteSLURM');
      $resource = TripalRemoteSLURM::byID($resource_id);
    }
  }
  else {
    $resource = new TripalRemoteSSH();
  }

  // Generate the form.
  $form['#tree'] = TRUE;

  // Add in the resource related objects for future reference.
  $form['resource'] = array(
    '#type'  => 'value',
    '#value'  => $resource,
  );

  $form['name'] = array(
    '#title'         => t('Resource Name'),
    '#type'          => 'textfield',
    '#description'   => t("Please provide a name that describes this resource."),
    '#default_value' => $resource->getName(),
    '#required'      => TRUE,
  );;
  $form['type'] = array(
    '#title'    => t('Resource Type'),
    '#type'     => 'select',
    '#options'  => array(
      'alone'   => 'Remote stand-alone computational server',
      'ge'      => 'GridEngine cluster',
      'pbs'     => 'PBS compatible cluster',
      'slurm'   => 'SLURM cluster',
    ),
    '#default_value' => $resource->getType(),
    '#required'      => TRUE,
  );
  $form['description'] = array(
    '#title'         => t('Resource Description'),
    '#type'          => 'textarea',
    '#description'   => t("Provide a description for this resource."),
    '#default_value' => $resource->getDescription(),
    '#required'      => TRUE,
    '#rows'          => 2,
  );
  $form['max_slots'] = array(
    '#title'            => t('Maximum number of job slots'),
    '#type'             => 'textfield',
    '#default_value'    => $resource->getMaxSlots(),
    '#element_validate' => array('element_validate_integer'),
    '#required'         => TRUE,
    '#description'      => t("Provide the maximum number of jobs that are
        allowed to be run in parallel at once."),
  );

  $rank = $resource->getRank();
  if ($rank == '') {
    $rank = TripalRemoteSSH::getNextRank();
  }
  else {
    $rank = ($rank == '0') ? 0 : intval($rank);
  }
  $form['rank'] = array(
    '#title'            => t('Rank'),
    '#type'             => 'textfield',
    '#default_value'    => $rank,
    '#element_validate' => array('element_validate_integer'),
    '#required'         => TRUE,
    '#description'      => t("Specify the rank of this resource.
        Jobs will be scheduled on resources with lower ranks first."),
  );
  $form['enabled'] = array(
    '#title'         => t('Is Enabled'),
    '#type'          => 'checkbox',
    '#default_value' => $resource->isEnabled(),
    '#description'   => t("Check if this resource should be available by
        default to all users?  If not users will be able
        to find the resource in a list but it will not show by
        default for selection."),
  );

  // Woring directory on this resource.
  $form['working_directory'] = array(
    '#title'         => t('Remote Working Directory'),
    '#type'          => 'textfield',
    '#default_value' => $resource->getWorkingDir(),
    '#description'   => t("Specify the path where the remote job will
        perform it's work.  The \"Test Remote Configuration\" button below will 
        attempt to create this directory if it does not already exist on the 
        remote machine (recommended). Within this directory, a 
        <code>tripal_remote_job</code> directory will be created and individual 
        jobs will each have their own directory here."),
  );

  // The connection section.
  $form['conn'] = array(
    '#type'         => 'fieldset',
    '#collapsed'    => FALSE,
    '#collapsible'  => TRUE,
    '#title'        => 'Connection',
  );
  if (!exec('whoami', $web_user))
        $web_user[0] = 'unknown. For Apache, it is probably <code>apache</code> or <code>www-data</code>; for nginx, it is probably <code>nginx</code>.';
  if (!exec('echo $HOME', $web_home)) {
          $web_home[0] = 'unknown.';
  }
  else {
      $web_home[0] = $web_home[0].'/.ssh';
  }
  $form['conn']['instructions'] = array(
    '#markup' =>  '<p>' . t('Enter the connection details below. A connection to'
            . ' the remote SSH server should be made prior to this point'
            . ' in order to ensure connection details are correct and to avoid'
            . ' man-in-the-middle attacks. The SSH key file should be within the'
            . ' web-user\'s ~/.ssh directory. ') . '</p><p>' . t('For reference, your web user '
            . 'is <code>') . $web_user[0] . t('</code> and its SSH directory is <code>') . $web_home[0] . t('</code>') ,
  );
  
  // Test Remote Configuration
  $form['create_ssh'] = array(
    '#type' => 'submit',
    '#name' => 'create_ssh',
    '#value' => t('Do this for me')
  );
  
  $hostname = $resource->getHostname();
  $form['conn']['hostname'] = array(
    '#title'         => t('Hostname'),
    '#type'          => 'textfield',
    '#description'   => t("The hostname of SSH"),
    '#default_value' => $hostname,
    '#required'      => TRUE,
  );

  $ssh_port = $resource->getSSHPort();
  $form['conn']['ssh_port'] = array(
    '#title'         => t('Port #'),
    '#type'          => 'textfield',
    '#description'   => t("Port number of SSH"),
    '#default_value' => $ssh_port,
    '#required'      => TRUE,
  );

  $username = $resource->getUsername();
  $form['conn']['username'] = array(
    '#title'         => t('Username'),
    '#type'          => 'textfield',
    '#description'   => t("Username of SSH"),
    '#default_value' => $username,
    '#required'      => TRUE,
  );

  // Add functional buttons.
  if ($resource->getResourceID()) {
    $form['button'] = array(
      '#type'  => 'submit',
      '#name'  => 'update',
      '#value' => t('Update Resource'),
    );
  }
  else {
    $form['button'] = array(
      '#type'  => 'submit',
      '#name'  => 'insert',
      '#value' => t('Add Resource'),
    );
  }
  
  // Test Remote Configuration
  $form['test_remote'] = array(
    '#type' => 'submit',
    '#name' => 'test_remote',
    '#value' => t('Test Remote Configuration (Optional)')
  );
  
  $form['#prefix'] = '<div id="tripal-remote-job-admin-resource-form">';
  $form['#suffix'] = '</div>';
  return $form;
}

/**
 * Validate the resource edit form.
 *
 * @param $form
 * @param $form_state
 */
function tripal_remote_job_admin_resource_form_validate($form, &$form_state) {

  // TODO: do not allow for changing of the 'type' for the local server as
  // we always need this resource.
}

/**
 * Submit the resource edit form.
 *
 * @param $form
 * @param $form_state
 */
function tripal_remote_job_admin_resource_form_submit($form, &$form_state) {
  // Test Remote button was clicked, don't submit quite yet
  if ($form_state['clicked_button']['#name'] == 'test_remote')
  {
      $form_state['rebuild'] = TRUE;
      
      // Gather the connection details from the form into an array
      $connection_details = array();
      $connection_details['remote_working_directory']   = $form_state['values']['working_directory'];
      $connection_details['hostname']                   = $form_state['values']['conn']['hostname'];
      $connection_details['ssh_port']                       = $form_state['values']['conn']['ssh_port'];
      $connection_details['username']                   = $form_state['values']['conn']['username'];
      // Call the tripal_remote_job_check_configuration function
      /* Reminder of possible return values:
       * 0 - Connection successful, remote working directory exists and is writeable    [directory_exists_is_writable]
       * 1 - Connection successful, remote working directory exists but is not writable [directory_exists_not_writable]
       * 2 - Connection successful, remote working directory did not exist, created     [directory_created]
       * 3 - Connection successful, remote working directory could not be created       [directory_failed]
       * 4 - Connection unsuccessful
       * 5 - Something went wrong prior to connection
       */
      
      $remote_status = tripal_remote_job_check_configuration($connection_details);
      switch ($remote_status)
      {
          case 0:
              drupal_set_message("Connection successful.");
              drupal_set_message("The remote working directory exists and is writable.");
              break;
          case 1:
              drupal_set_message("Connection successful.");
              drupal_set_message("The remote working directory exists but is not writable.", 'warning');
              break;
          case 2:
              drupal_set_message("Connection successful.");
              drupal_set_message("The remote working directory did not exist - it has beeen created");
              break;
          case 3:
              drupal_set_message("Connection successful.");
              drupal_set_message("The remote working directory could not be created", 'error');
              break;
          case 4:
              // @todo - Make this more informative - which details are invalid?
              //         This may be handled in the hook_validate section
              drupal_set_message("Connection details are invalid, please review them", 'error');
              break;
          case 5:
              // Catch-all for other issues 
              drupal_set_message("Something went wrong prior to connection", 'error');
      }
      
      
  }
  else if ($form_state['clicked_button']['#name'] == 'create_ssh')
  {
      $form_state['rebuild'] = TRUE;
      
      exec('[ -f ~/test ] || mkdir test');
      exec('[ -f ~/test/thing.txt || touch ~/test/thing.txt');
  }
  // Normal submit button pressed, initiate submission
  else
  {
    module_load_include('inc', 'tripal_remote_job', 'includes/TripalRemoteResource');

    $resource = $form_state['values']['resource'];

    // Set properties of TripalRemoteResource.
    $resource->setName(trim($form_state['values']['name']));
    $resource->setDescription(trim($form_state['values']['description']));
    $resource->setType(trim($form_state['values']['type']));
    $resource->setRank(trim($form_state['values']['rank']));
    $resource->setMaxSlots(trim($form_state['values']['max_slots']));
    $resource->setWorkingDir(trim($form_state['values']['working_directory']));

    if ($form_state['values']['enabled'] == 1) {
      $resource->enable();
    }
    else {
      $resource->disable();
    }

    // Updates TripalRemoteResource.
    if ($form_state['clicked_button']['#name'] == "update") {
      if ($resource->update()) {
        drupal_set_message('Resource succesfully updated.', 'status');
      }
      else {
        drupal_set_message('Resource could not be updated.', 'error');
      }
    }

    // Adds TripalRemoteResource.
    else if ($form_state['clicked_button']['#name'] == "insert") {
      if ($resource->insert()) {
        drupal_set_message('Resource succesfully added.', 'status');
      }
      else {
        drupal_set_message('Resource could not be added.', 'error');
      }
    }

    // Update resource ID.
    $resource_id = $resource->getResourceID();

    // Adds or Updates remote connection information.
    $hostname = trim($form_state['values']['conn']['hostname']);
    $ssh_port = trim($form_state['values']['conn']['ssh_port']);
    $username = trim($form_state['values']['conn']['username']);

    // Updates TripalRemoteSSH.
    $remote_resource = TripalRemoteSSH::byID($resource_id);
    if ($remote_resource) {
      $remote_resource->setHostname($hostname);
      $remote_resource->setSSHPort($ssh_port);
      $remote_resource->setUsername($username);
      $remote_resource->update();
    }
    // Adds TripalRemoteSSH.
    else {
      $remote_resource = new TripalRemoteSSH(array(
        'resource_id' => $resource->getResourceID(),
        'hostname'    => $hostname,
        'ssh_port'    => $ssh_port,
        'username'    => $username,
      ));
      $remote_resource->insert();
    }

    $form_state['redirect'] = 'admin/tripal/extension/remote-resources';
  }
}

/**
 * The resource delete form.
 *
 * @param $form
 * @param $form_state
 * @param $resource_id
 */
function tripal_remote_job_admin_resource_delete_form($form, $form_state, $resource_id = NULL) {

  // Get selected resource.
  module_load_include('inc', 'tripal_remote_job', 'includes/TripalRemoteResource');
  $resource = TripalRemoteResource::byID($resource_id);

  $form['resource'] = array(
    '#type' => 'value',
    '#value' => $resource,
  );

  return confirm_form($form,
      t('Please confirm you would like to delete the resource named: ' . $resource->getName()),
      'admin/tripal/extension/remote-resources',
      t('This action cannot be undone.'),
      t('Yes, Delete'),
      t('No, Cancel')
  );
}

/**
 * Submit the resource delete form.
 *
 * @param $form
 * @param $form_state
 */
function tripal_remote_job_admin_resource_delete_form_submit($form, $form_state) {
  $resource = $form_state['values']['resource'];
  if ($resource->delete()) {
    drupal_set_message('Resource succesfully deleted.', 'status');
  }
  else {
    drupal_set_message('Resource could not be deleted.', 'error');
  }
  drupal_goto('admin/tripal/extension/remote-resources');
}
