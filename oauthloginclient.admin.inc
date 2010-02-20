<?php

/**
 * Menu system callback for the OAuth login client settings index page.
 */
function _oauthloginclient_admin_index() {
  return theme('oauthloginclient_admin_index', oauthloginclient_get_servers());
}

/**
 * Menu system callback for the add server form.
 */
function _oauthloginclient_admin_add_server($form_state) {
  $form = array();

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('The machine-readable name of the OAuth login server.'),
    '#size' => 32,
    '#maxlength' => 100,
    '#required' => TRUE,
  );

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('The title of the OAuth login server.'),
    '#size' => 32,
    '#maxlength' => 255,
    '#required' => TRUE,
  );

  $form['url'] = array(
    '#type' => 'textfield',
    '#title' => t('Url'),
    '#description' => t('A url to the OAuth login server, typically to the front page.'),
    '#size' => 32,
    '#maxlength' => 255,
    '#required' => TRUE,
  );

  $form['endpoint'] = array(
    '#type' => 'textfield',
    '#title' => t('Endpoint'),
    '#description' => t('The api endpoint for the user info action.'),
    '#size' => 32,
    '#maxlength' => 255,
    '#required' => TRUE,
  );

  $form['oauth_consumer_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Login consumer key'),
    '#description' => t('The consumer key for this site on the OAuth login server.'),
    '#size' => 32,
    '#maxlength' => 32,
    '#required' => TRUE,
  );

  $form['oauth_consumer_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('Login consumer secret'),
    '#description' => t('The consumer secret for this site on the OAuth login server'),
    '#size' => 32,
    '#maxlength' => 32,
    '#required' => TRUE,
  );

  $form['create'] = array(
    '#type' => 'submit',
    '#value' => t('Create'),
  );

  return $form;
}

/**
 * Submit handler for _oauthloginclient_admin_add_server().
 */
function _oauthloginclient_admin_add_server_submit($form, $form_state) {
  global $user;

  $values = $form_state['values'];
  oauthloginclient_add_server($values);

  // Save the consumer token so that we can get the secret later
  $c_token = DrupalOAuthConsumer::load($values['oauth_consumer_key'], FALSE);
  if ($c_token) {
    $c_token->secret = $values['consumer_secret'];
    $c_token->provider_url = trim($values['url'], '/');
    $c_token->write(TRUE);
  }
  else {
    $c_token = new DrupalOAuthConsumer($values['oauth_consumer_key'], $values['oauth_consumer_secret'], 'oob', FALSE, array(
      'configuration' => array(
        'provider_url' => trim($values['url'], '/'),
      )
    ));
    $c_token->write();
  }

  drupal_set_message(t('The server was added'), 'info');
  drupal_goto('admin/settings/oauthloginclient/index');
}

/**
 * Menu system callback for the edit server form.
 *
 * @param array $form_state
 * @param int $sid
 *  sid of server to edit.
 * @return array
 *  Form definition.
 */
function _oauthloginclient_admin_edit_server($form_state, $sid) {
  // Fetch server info
  $servers = oauthloginclient_get_servers();
  $server = $servers[$sid];

  $form = array();

  $form['sid'] = array(
    '#type' => 'value',
    '#value' => $sid,
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('The name of the OAuth login server.'),
    '#size' => 32,
    '#maxlength' => 100,
    '#required' => TRUE,
    '#default_value' => $server['name'],
  );

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('The name of the OAuth login server.'),
    '#size' => 32,
    '#maxlength' => 255,
    '#required' => TRUE,
    '#default_value' => $server['title'],
  );

  $form['url'] = array(
    '#type' => 'textfield',
    '#title' => t('Url'),
    '#description' => t('A url to the OAuth login server, typically to the front page.'),
    '#size' => 32,
    '#maxlength' => 255,
    '#required' => TRUE,
    '#default_value' => $server['url'],
  );

  $form['endpoint'] = array(
    '#type' => 'textfield',
    '#title' => t('Endpoint'),
    '#description' => t('The api endpoint for the user info action.'),
    '#size' => 32,
    '#maxlength' => 255,
    '#required' => TRUE,
    '#default_value' => $server['endpoint'],
  );

  $form['oauth_consumer_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Consumer key'),
    '#description' => t('The consumer key for this site on the OAuth login server.'),
    '#size' => 32,
    '#maxlength' => 32,
    '#required' => TRUE,
    '#default_value' => $server['oauth_consumer_key'],
  );

  $form['oauth_consumer_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('Consumer secret'),
    '#description' => t('The consumer secret for this site on the OAuth login server'),
    '#size' => 32,
    '#maxlength' => 32,
    '#required' => TRUE,
    '#default_value' => DrupalOAuthConsumer::load($server['oauth_consumer_key'], FALSE)->secret,
  );

  $form['update'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  return $form;
}

/**
 * Submit handler for _oauthloginclient_admin_edit_server().
 */
function _oauthloginclient_admin_edit_server_submit($form, &$form_state) {
  $values = $form_state['values'];
  $servers = oauthloginclient_get_servers();

  // Get original
  $server = $servers[$values['sid']];
  $consumer = DrupalOAuthConsumer::load($server['oauth_consumer_key'], FALSE);

  if ($consumer) {
    $consumer->delete();
  }

  // Create new consumer token
  $consumer = new DrupalOAuthConsumer($values['oauth_consumer_key'], $values['oauth_consumer_secret'], 'oob', FALSE, array(
    'configuration' => array(
      'provider_url' => trim($values['url'], '/'),
    ),
  ));
  $consumer->write();

  // Update server
  oauthloginclient_update_server($values);
  drupal_set_message(t('The server was updated'), 'info');
  drupal_goto('admin/settings/oauthloginclient/index');
}
