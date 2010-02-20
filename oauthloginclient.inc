<?php

/**
 * @file
 *
 */

/**
 * Error code used when the login fails for unknown reasons.
 */
define('OAUTHLOGIN_ERROR', 0);

/**
 * Error code used when the login fails because the external user information
 * is incomplete.
 */
define('OAUTHLOGIN_ERROR_MISSING_INFO', 1);

/**
 * Error code used when the login fails because the email already is
 * registered.
 */
define('OAUTHLOGIN_ERROR_EMAIL_REGISTERED', 2);

/**
 * Fetches a request token from the OAuth login server and redirects to the
 * authorization page.
 *
 * @param int $sid
 *  The sid of the OAuth login server to connect to.
 * @return void
 */
function _oauthloginclient_login($sid) {
  $servers = oauthloginclient_get_servers();
  $server = $servers[$sid];
  $consumer_token = DrupalOAuthConsumer::load($server['oauth_consumer_key'], FALSE);

  $client = new DrupalOAuthClient($consumer_token);
  $request_token = $client->getRequestToken();
  $request_token->write();
  $auth_url = $client->getAuthorizationUrl();

  $_SESSION['oauthlogin_request_key'] = $request_token->key;

  drupal_goto($auth_url);
}

/**
 * Fetches external user information.
 *
 * @param array $login_server
 *  Login server information array.
 * @param DrupalOAuthToken $access_token
 *  The access token.
 * @param DrupalOAuthConsumer $consumer_token
 *  Optional. The consumer token.
 * @return array
 *  The external user information.
 */
function _oauthloginclient_fetch_external_user($login_server, $access_token, $consumer_token=NULL) {
  // Load the consumer token if needed
  if (!$consumer_token) {
    $consumer_token = DrupalOAuthConsumer::load($access_token->consumer_key, FALSE);
  }

  // Set up the rest client
  $auth = new HttpClientOAuth($consumer_token, $access_token, new OAuthSignatureMethod_HMAC('sha1'));
  $client = new HttpClient($auth, new HttpClientBaseFormatter(HttpClientBaseFormatter::FORMAT_PHP));

  // Fetch the external user
  $info = FALSE;
  try {
    $info = (array)$client->post($login_server['endpoint'], array());
  } catch (Exception $e) {
    drupal_set_message(t('Failed to fetch user info for login: @message', array(
      '@message' => $e->getMessage(),
    )), 'error');
  }
  return $info;
}


/**
 * Logs in a external user
 *
 * @param array $server
 *  The OAuth login server information array.
 * @param array $external_user
 *  The external user information array.
 * @return void
 * @throws Exception
 *  An exception is thrown if login fails because of incomplete external user
 *  information (code: OAUTHLOGIN_ERROR_MISSING_INFO), if a user with the same
 *  email already is registered (code: OAUTHLOGIN_ERROR_EMAIL_REGISTERED). If
 *  the failure reason is unknown the code OAUTHLOGIN_ERROR is used.
 */
function _oauthloginclient_login_or_register_external_user($server, $external_user) {
  global $user;

  // Check for required attributes
  $required = array('uid', 'name', 'mail');
  foreach ($required as $attribute) {
    if (empty($external_user[$attribute])) {
      throw new Exception(t('The OAuth login server returned incomplete information, missing !attribute', array(
        '!attribute' => $attribute,
      )), OAUTHLOGIN_ERROR);
    }
  }

  // Check if a connection to a internal user already exists
  $internal = db_fetch_array(db_query("SELECT uid FROM {oauthloginclient_user} WHERE euid=%d AND name='%s'", array(
    ':eid' => $external_user['uid'],
    ':name' => $server['name'],
  )));
  // If we already have a mapping for the external user
  if (!empty($internal)) {
    $user = user_load($internal);

    sess_regenerate();
    module_invoke_all('user', 'login', NULL, $user);
  }
  else { // If a new internal user has to be created
    // Check so that we don't get a name or email collision
    $tres = db_query("SELECT name, mail FROM {users} WHERE name='%s' OR mail='%s'", array(
      ':name' => $external_user['name'],
      ':mail' => $external_user['mail'],
    ));
    while ($taken = db_fetch_array($tres)) {
      if ($taken['name'] == $external_user['name']) {
        // Handle name collisions by appending @[server name] to the user name
        $external_user['name'] .= '@' . $server['name'];
      }
      else if ($taken['email'] == $external_user['email']) {
        throw new Exception(t('There is already a registered user with the email !email', array(
          '!email' => $taken['email'],
        )), OAUTHLOGIN_ERROR_EMAIL_REGISTERED);
      }
    }

    // Create a internal user info array from the external info.
    $internal = array(
      'pass' => user_password(32),
    ) + $external_user;
    // Filter out uid and roles from the external user and set
    // other needed attributes.
    unset($internal['uid']);
    unset($internal['roles']);
    $internal['status'] = TRUE;

    // Save the user and add a entry for the external user.
    $user = user_save((object)$internal, $internal);
    drupal_write_record('oauthloginclient_user', $values=array(
      'euid' => $external_user['uid'],
      'name' => $server['name'],
      'uid' => $user->uid,
    ));

    sess_regenerate();
    module_invoke_all('user', 'login', NULL, $user);
  }

  // Check if the login was successful
  if (!$user->uid) {
    throw new Exception(t('Login failed because of a unknown reason'), OAUTHLOGIN_ERROR);
  }
}