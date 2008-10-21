<?php

/**
 * @file
 * Rules specific functions that expose content_access' API.
 */


/**
 * Adds the role based settings to the form.
 */
function content_access_rules_role_based_form($settings, &$form) {
  module_load_include('inc', 'content_access', 'content_access.admin');
  $form['#includes'][] = './'. drupal_get_path('module', 'content_access') .'/content_access.admin.inc';
  $form += content_access_role_based_form($settings);
}

/**
 * Parse submitted settings for per-node form into internal format.
 */
function content_access_parse_settings(&$settings, $form, $form_state) {
  foreach (_content_access_get_operations() as $op) {
    $settings[$op] = array_filter($form_state['values'][$op]);
  }
}

/**
 * Verifies that per content settings are activated for the given node.
 */
function _content_access_rules_check_setting($node) {
  if (!content_access_get_settings('per_node', $node->type)) {
    rules_log(t("Can't set per content permissions for content type @type. Make sure to have per content settings activated for content types you want to alter access control for.", array('@type' => node_get_types('name', $node->type))), TRUE);
    return FALSE;
  }
  return TRUE;
}


/**
 * Implementation of hook_action_info().
 */
function content_access_rules_action_info() {
  return array(
    'content_access_action_grant_node_permissions' => array(
      'label' => t('Grant content permissions by role'),
      'arguments' => array(
        'node' => array('type' => 'node', 'label' => t('Content')),
      ),
      'module' => 'Content access',
    ),
    'content_access_action_revoke_node_permissions' => array(
      'label' => t('Revoke content permissions by role'),
      'arguments' => array(
        'node' => array('type' => 'node', 'label' => t('Content')),
      ),
      'module' => 'Content access',
    ),
    'content_access_action_reset_node_permissions' => array(
      'label' => t('Reset content permissions'),
      'arguments' => array(
        'node' => array('type' => 'node', 'label' => t('Content')),
      ),
      'module' => 'Content access',
    ),
  );
}

/**
 * Action implementation: Grant permissions for a node.
 */
function content_access_action_grant_node_permissions($node, $settings) {
  if (_content_access_rules_check_setting($node)) {
    $ca_settings = array();
    foreach (_content_access_get_operations() as $op) {
      $ca_settings[$op] = array_keys(array_flip(content_access_per_node_setting($op, $node)) + array_flip($settings[$op]));
    }
    content_access_save_per_node_settings($node, $ca_settings);
    // A following node_save() updates the grants for us.
    return array('node' => $node);
  }
}

function content_access_action_grant_node_permissions_form($settings, &$form) {
  content_access_rules_role_based_form($settings, $form);
}
function content_access_action_grant_node_permissions_submit(&$settings, $form, $form_state) {
  content_access_parse_settings($settings, $form, $form_state);
}

/**
 * Action implementation: Revoke permissions for a node.
 */
function content_access_action_revoke_node_permissions($node, $settings) {
  if (_content_access_rules_check_setting($node)) {
    $ca_settings = array();
    foreach (_content_access_get_operations() as $op) {
      $ca_settings[$op] = array_diff(content_access_per_node_setting($op, $node), $settings[$op]);
    }
    content_access_save_per_node_settings($node, $ca_settings);
    // A following node_save() updates the grants for us.
    return array('node' => $node);
  }
}

function content_access_action_revoke_node_permissions_form($settings, &$form) {
  content_access_rules_role_based_form($settings, $form);
}
function content_access_action_revoke_node_permissions_submit(&$settings, $form, $form_state) {
  content_access_parse_settings($settings, $form, $form_state);
}


/**
 * Action implementation: Reset permissions for a node.
 */
function content_access_action_reset_node_permissions($node) {
  if (_content_access_rules_check_setting($node)) {
    content_access_delete_per_node_settings($node);
    // A following node_save() updates the grants for us.
    return array('node' => $node);
  }
}

/**
 * Implementation of hook_condition_info().
 */
function content_access_rules_condition_info() {
  return array(
    'content_access_condition_check_permissions' => array(
    'label' => t('Check role based settings'),
    'arguments' => array(
      'node' => array('type' => 'node', 'label' => t('Content')),
    ),
    'help' => t('Evaluates to TRUE, if content access allows all selected operations for the given roles.'),
    'module' => 'Content access',
    ),
  );
}

/**
 * Condition implementation: Check if node has permissions.
 * 
 * @note
 *   This will only check for the existence of permissions, not the
 *   absence of. I.e. a rule that checks just for authenticated write
 *   will return TRUE for a node that allows authenticated and anonymous
 *   write.
 */
function content_access_condition_check_permissions($node, $settings) {
  if (_content_access_rules_check_setting($node)) {
    // Compare our settings with node's settings.
    foreach (_content_access_get_operations() as $op) {
      $expect_roles = $settings[$op];
      $current_roles = content_access_per_node_setting($op, $node);
      if (count(array_diff($expect_roles, $current_roles)) != 0) {
        return FALSE;
      }
    }
    return TRUE;
  }
  return FALSE;
}

function content_access_condition_check_permissions_form($settings, &$form) {
  content_access_rules_role_based_form($settings, $form);
}
function content_access_condition_check_permissions_submit(&$settings, $form, $form_state) {
  content_access_parse_settings($settings, $form, $form_state);
}