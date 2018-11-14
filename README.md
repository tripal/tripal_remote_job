# tripal_remote_job
Allows Tripal to run jobs on other machines or clusters

Ported from GenSAS to work with Tripal

## Status
  - [ ] SSH
    - [x] Initiate Job
    - [ ] Get status of job (i.e. percent complete) - will not do
    - [x] Get results of job
  - [ ] PBS
  - [ ] SLURM
  - [ ] GridEngine
  - [x] Email Notification
    - [x] Job initiated (generic)
    - [x] Job complete (generic)
    - [x] Custom email specified by origin module
  - [x] Allow user to specify SSH Key locations
  - [ ] Toggle ability of user (admin?) to prevent remote files from being deleted (Default: disabled, files will always be deleted)
  - [ ] API argument to specify target destination for job if multiple (make some logic to dynamically assign?)
  
## Requirements
Tripal 3 (Tested with Tripal 3 [7.x-3.0-rc1])
libraries module
drushd module

## Installation
1. Install module in same way as any other module
2. From Admin menu, visit 'Tripal > Extensions > Remote Computational Resources' to set up one or more remote servers. See the 'Remote Resource Configuration' section below for specific information for each type of remote resource. 
3. Have your module call tripal_add_remote_job() with following arguments:
   -Argument:    Job Name
    Type:        String
    Required:    Yes
   -Argument:    Command
    Type:        String
    Required:    Yes
   -Argument:    Origin Module - Used so that the Mail functionality
    Type:        String
    Required:    Yes
   -Argument:    Files
    Type:        Array
    Required:    Optional (Required if specifying Email Notification)
   -Argument:    Email Notification
    Type:        Boolean
    Required:    Optional
       
 ##### Examples:
    
 tripal_add_remote_job("myCoolRemoteJob", "uptime >> coolLevel.txt","my_module" array("/local/Path/to/coolLevel.txt"), FALSE);
    Executes regular system command 'uptime' and appends its output to coolLevel.txt without notification via Email
    
 tripal_add_remote_job("simpleUptime", "uptime", "my_module");
    Executes regular system command 'uptime'. 
    Its output will be returned within STDOUT.txt. No additional files were used and no email notification was sent.

## Remote Resource Configuration
#### SSH
1. **Generate an SSH key pair**
    - The main hurdle of configuring a remote SSH server is to generate an SSH key pair for the webserver and the remote SSH server. Instructions may be found online for your specific system (Centos, Debian, Mac OS, etc.).
    - The SSH key pair must be generated for the user who you have set to run the Tripal Job Daemon (i.e. whichever user runs `drush trpjob-daemon start` or similar). **This is important to note**. *In most cases*, whenever the Tripal Job Daemon must be started, it must be done so by the same user every time for the SSH key authentication to work correctly. You may want to create a system user specifically for this purpose in cases where there are multiple admins who need to stop/start/restart the Tripal Job Daemon for any reason. This also means that attempting to use the web interface (`admin/tripal/tripal_jobs`) to launch jobs that require Tripal Remote Job will not work (these are executed as the webserver user, typically `www-data` for Apache or `nginx` for NGINX).
2. **Ensure authentication has succeeded, authorize any keys** 
    - After generating the key pair and installing the public key on the remote server, the connection must be made at least one time in order to authorize the key and make sure there is no man-in-the-middle.
3. **Use the form to fill in connection details** 
    - When configuring a remote SSH connection, the **SSH Key Location** must be set to the full file path, including the key itself, of the private SSH key. For example, `/home/admin/.ssh/id_rsa`. 
#### PBS
Will be updated when functionality becomes available
#### SLURM
Will be updated when functionality becomes available
#### GridEngine
Will be updated when functionality becomes available

## Remote Resource Requirements
Refer to this section when configuring your remote resources
#### SSH / Stand-alone Computational resource
  -Any commands to be called must either exist on the $PATH of the configured remote user or be passed along with the contents of the job

## Mail Functionality
When creating a job, Tripal Remote Job is aware of what module (origin module) has spawned the job (via the API call).
It will then query that module to see if there are any specific mail functionality required for notifying the user in two conditions:
    -Job is launched
    -Job has completed
These require that the module's hook_mail function has provisions for the following keys:
    `job-started`
    `job-completed`
If these keys exist, the module's hook_mail function will be used. Else, Tripal Remote Job will send a generic email about job status. This functionality gives the origin module the ability to specify a template for the emails to be sent when a job starts or finishes.
**Modules that use Tripal Remote job are not required to provide custom mail functionality**