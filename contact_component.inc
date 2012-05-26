<?php

/**
 * @file
 * CiviCRM contact webform component.
 */

module_load_include('inc', 'webform_civicrm', 'webform_civicrm_utils');

/**
 * Implements _webform_defaults_component().
 */
function _webform_defaults_civicrm_contact() {
  return array(
    'name' => '',
    'form_key' => NULL,
    'pid' => 0,
    'weight' => 0,
    'value' => '',
    'mandatory' => 0,
    'extra' => array(
      'items' => '',
      'width' => '',
      'field_prefix' => '',
      'field_suffix' => '',
      'search_prompt' => t('Search for an existing contact'),
      'none_prompt' => t('Create new contact'),
      'allow_create' => TRUE,
      'widget' => 'autocomplete',
      'unique' => 0,
      'title_display' => 0,
      'description' => '',
      'hide_fields' => array('name'),
      'attributes' => array(),
      'private' => FALSE,
      'default' => '',
      'default_contact_id' => '',
      'default_relationship' => '',
      'filters' => array(
        'contact_sub_type' => 0,
        'group' => array(),
        'tag' => array(),
        'check_permissions' => 0,
      ),
    ),
  );
}

/**
 * Implements _webform_edit_component().
 */
function _webform_edit_civicrm_contact($component) {
  civicrm_initialize();
  $form = array();
  $node = node_load($component['nid']);
  if (empty($node->webform_civicrm)) {
    drupal_set_message(t('CiviCRM processing is not enabled for this webform.'), 'error');
    return;
  }
  list($contact_types, $sub_types) = webform_civicrm_get_contact_types();
  $data = $node->webform_civicrm['data'];
  list(, $c, ) = explode('_', $component['form_key'], 3);
  $contact_type = $data['contact'][$c]['contact'][1]['contact_type'];
  // Check for required library
  $path = libraries_get_path('tokeninput');
  if (!is_file(DRUPAL_ROOT . '/' . $path . '/src/jquery.tokeninput.js')) {
    drupal_set_message(t('Could not find the jQuery TokenInput library. Please <a href="http://loopj.com/jquery-tokeninput">download it</a> and extract as the folder "sites/all/libraries/tokeninput". This is required for the autocomplete to function.'), 'warning');
  }
  // Load scripts & css
  else {
    $form['#attached']['js'][] = $path . '/src/jquery.tokeninput.js';
    $callback_path = '"' . url('webform-civicrm/js/' . $node->nid . '-' . $component['cid'], array('alias' => TRUE, 'query' => array('admin' => 1))) . '"';
    $settings = '{
      queryParam: "str",
      hintText: "' . t('Choose @type', array('@type' => $contact_types[$contact_type])) . '",
      noResultsText: "' . t('None found') . '",
      searchingText: "' . t('Searching...') . '",
      tokenLimit: 1,
      prePopulate: prep
    }';
    $js = '
    jQuery(document).ready(function() {
      var prep = wfCiviContact.init(' . $callback_path . ');
      jQuery("#default-contact-id").tokenInput(' . $callback_path . ', ' . $settings . ');
    });';
    $form['#attached']['js'][$js] = array('type' => 'inline');
    $form['#attached']['css'][] = drupal_get_path('module', 'webform_civicrm') . '/token-input.css';
  }
  $form['#attached']['js'][] = drupal_get_path('module', 'webform_civicrm') . '/contact_component.js';
  $form['display']['hide_fields'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => t('Fields to Hide'),
    '#default_value' => $component['extra']['hide_fields'],
    '#description' => t('When an existing contact is selected, which fields should the user be allowed to edit and which should be hidden?'),
    '#options' => webform_civicrm_contact_fields($node, $c),
    '#weight' => -8,
    '#parents' => array('extra', 'hide_fields'),
  );
  $form['display']['widget'] = array(
    '#type' => 'select',
    '#title' => t('Form Widget'),
    '#default_value' => $component['extra']['widget'],
    '#options' => array('autocomplete' => t('Autocomplete'), 'select' => t('Select List'), 'hidden' => t('Hidden')),
    '#weight' => -9,
    '#parents' => array('extra', 'widget'),
    '#description' => '<ul>
      <li>' . t('Autocomplete will suggest names of contacts as the user types. Good for large numbers of contacts.') . '</li>
      <li>' . t('A select list will show all possible contacts in a dropdown menu. Good for small lists.') . '</li>
      <li>' . t('A hidden element will not allow the user to make a choice.') . '</li>
      </ul>',
  );
  $form['display']['search_prompt'] = array(
    '#type' => 'textfield',
    '#title' => t('Search Prompt'),
    '#default_value' => $component['extra']['search_prompt'],
    '#description' => t('Text the user will see before selecting a contact.'),
    '#size' => 60,
    '#maxlength' => 1024,
    '#weight' => -1,
    '#required' => TRUE,
    '#parents' => array('extra', 'search_prompt'),
  );
  $form['display']['none_prompt'] = array(
    '#type' => 'textfield',
    '#title' => t('Create/Not Found Prompt'),
    '#default_value' => $component['extra']['none_prompt'],
    '#description' => t('If you have exposed a name or email field, this text should prompt the user to create a new contact. Otherwise this text should tell the user that no search results were found.'),
    '#size' => 60,
    '#maxlength' => 1024,
    '#weight' => 0,
    '#required' => TRUE,
    '#parents' => array('extra', 'none_prompt'),
  );
  $form['validation']['unique'] = array(
    '#type' => 'checkbox',
    '#title' => t('Unique'),
    '#return_value' => 1,
    '#description' => t('Require this field to be unique for every submission. The same contact may not be entered twice.'),
    '#weight' => 1,
    '#default_value' => $component['extra']['unique'],
    '#parents' => array('extra', 'unique'),
  );
  $form['extra']['allow_create'] = array(
    '#type' => 'hidden',
    '#value' => webform_civicrm_name_field_exists(webform_civicrm_enabled_fields($node), $c),
  );
  $form['defaults'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default value'),
    '#description' => t('Should the form be pre-populated with an existing contact?') . '<br />',
    '#collapsible' => TRUE,
  );
  $form['defaults']['#description'] .= t('<strong>Note:</strong> This setting will be overridden if a contact id is passed in the url, i.e. !url', array('!url' => "cid$c=123"));
  $form['defaults']['default'] = array(
    '#type' => 'select',
    '#title' => t('Set default contact from'),
    '#options' => array('contact_id' => t('Specified Contact')),
    '#empty_option' => t('- None -'),
    '#default_value' => $component['extra']['default'],
    '#parents' => array('extra', 'default'),
  );
  if ($c == 1 && $contact_type == 'individual') {
    $form['defaults']['default']['#options']['user'] = t('Current User');
  }
  elseif ($c > 1) {
    $form['defaults']['default']['#options']['relationship'] = t('Relationship to Contact 1');
    $form['defaults']['default_relationship'] = array(
      '#type' => 'select',
      '#title' => t('Specify Relationship'),
      '#options' => webform_civicrm_get_contact_relationship_types($contact_type, $data['contact'][$c]['contact'][1]['contact_type'], $data['contact'][1]['contact'][1]['contact_sub_type'], $data['contact'][1]['contact'][1]['contact_sub_type']),
      '#default_value' => $component['extra']['default_relationship'],
      '#parents' => array('extra', 'default_relationship'),
    );
  }
  $form['defaults']['default_contact_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Contact'),
    '#id' => 'default-contact-id',
    '#parents' => array('extra', 'default_contact_id'),
  );
  if ($cid = $component['extra']['default_contact_id']) {
    if ($name = webform_civicrm_existing_contact_access($node, $component, array('check_permissions' => TRUE), $cid)) {
      $form['defaults']['default_contact_id']['#default_value'] = $cid;
      $form['defaults']['default_contact_id']['#attributes'] = array(
        'data-civicrm-name' => $name,
        'data-civicrm-id' => $cid,
      );
    }
  }
  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filters'),
    '#description' => t('Use filters to limit the list of contacts shown to the user.'),
    '#parents' => array('extra', 'filters'),
    '#tree' => TRUE,
    '#collapsible' => TRUE,
  );
  if (!empty($sub_types[$contact_type])) {
    $form['filters']['contact_sub_type'] = array(
      '#type' => 'select',
      '#title' => t('Type of @contact', array('@contact' => $contact_types[$contact_type])),
      '#options' => array(t('- Any -')) + $sub_types[$contact_type],
      '#default_value' => $component['extra']['filters']['contact_sub_type'],
    );
  }
  $form['filters']['group'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => t('Groups'),
    '#options' => webform_civicrm_get_options('group'),
    '#default_value' => $component['extra']['filters']['group'],
    '#description' => t('Listed contacts must be members of at least one of the selected groups (leave blank to not filter by group).'),
  );
  $form['filters']['tag'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => t('Tags'),
    '#options' => webform_civicrm_get_options('tag'),
    '#default_value' => $component['extra']['filters']['tag'],
    '#description' => t('Listed contacts must be have at least one of the selected tags (leave blank to not filter by tag).'),
  );
  $form['filters']['check_permissions'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enforce Permissions'),
    '#default_value' => $component['extra']['filters']['check_permissions'],
    '#description' => t('Only show contacts the acting user has permission to see in CiviCRM.'),
  );
  return $form;
}

/**
 * Implements _webform_render_component().
 */
function _webform_render_civicrm_contact($component, $value = NULL, $filter = TRUE) {
  $node = isset($component['nid']) ? node_load($component['nid']) : NULL;
  civicrm_initialize();
  $element = array(
    '#weight' => $component['weight'],
    '#attributes' => $component['extra']['attributes'],
    '#theme_wrappers' => array('webform_element'),
  );
  if ($component['extra']['widget'] != 'hidden') {
    $element += array(
      '#title' => $filter ? _webform_filter_xss($component['name']) : $component['name'],
      '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
      '#required' => $component['mandatory'],
      '#description' => $filter ? _webform_filter_descriptions($component['extra']['description'], $node) : $component['extra']['description'],
      '#translatable' => array('title', 'description'),
    );
  }
  list(, $c,) = explode('_', $component['form_key'], 3);
  $selector = '\'#webform-client-form-' . $component['nid'] . ' :text.civicrm-enabled[id$="' . str_replace('_', '-', $component['form_key']) . '"]\'';
  $callback_path = '"' . url('webform-civicrm/js/' . $node->nid . '-' . $component['cid'], array('alias' => TRUE)) . '"';
  if ($component['extra']['widget'] == 'autocomplete') {
    $element['#type'] = 'textfield';
    if ($node) {
      $path = libraries_get_path('tokeninput');
      $element['#attached']['js'][] = $path . '/src/jquery.tokeninput.js';
      $element['#attached']['css'][] = drupal_get_path('module', 'webform_civicrm') . '/token-input.css';
      $settings = '{
        queryParam: "str",
        hintText: "' . $component['extra']['search_prompt'] . '",
        noResultsText: "' . $component['extra']['none_prompt'] . '",
        searchingText: "' . t('Searching...') . '",
        tokenLimit: 1,
        onAdd: function(item) {wfCivi.existingSelect(' . $c . ', ' . $node->nid . ', ' . $callback_path . ', toHide, item.id);},
        onDelete: function(item) {wfCivi.existingSelect(' . $c . ', ' . $node->nid . ', ' . $callback_path . ', toHide, "");},
        prePopulate: prep
      }';
      $js = '
      (function ($) {
        $(document).ready(function() {
          var toHide = ' . json_encode(array_values($component['extra']['hide_fields'])) . ';
          var selector = ' . $selector . ';
          var prep = wfCivi.existingInit(' . $c . ', ' . $node->nid . ', ' . $callback_path . ', toHide, selector);
          jQuery(selector).tokenInput(' . $callback_path . ', ' . $settings . ');
        });
      })(jQuery);';
      $element['#attached']['js'][$js] = array('type' => 'inline');
    }
  }
  elseif ($component['extra']['widget'] == 'select') {
    $element['#type'] = 'select';
    $element['#empty_option'] = $component['extra']['search_prompt'];
    if ($node) {
      $element['#options'] = webform_civicrm_existing_contact_search($node, $component, $component['extra']['filters']);
      $js = '
      (function ($) {
        $(document).ready(function() {
          var toHide = ' . json_encode(array_values($component['extra']['hide_fields'])) . ';'
          . $selector . '.change(function() {
            wfCivi.existingSelect(' . $c . ', ' . $node->nid . ', ' . $callback_path . ', toHide, $(this).val());
          });
          wfCivi.existingInit(' . $c . ', ' . $node->nid . ', ' . $callback_path . ', toHide);
        });
      })(jQuery);';
      $element['#attached']['js'][$js] = array('type' => 'inline');
    }
  }
  else {
    $element['#type'] = 'hidden';
    if (!empty($value[0])) {
      $element['#value'] = $value[0];
    }
  }
  // Enforce uniqueness.
  if ($component['extra']['unique']) {
    $element['#element_validate'][] = 'webform_validate_unique';
  }

  if (!empty($value[0])) {
    $element['#default_value'] = $cid = $value[0];
    if ($name = webform_civicrm_existing_contact_access($node, $component, $component['extra']['filters'], $cid)) {
      $element['#attributes']['data-civicrm-name'] = $name;
      $element['#attributes']['data-civicrm-id'] = $cid;
    }
  }

  return $element;
}

/**
 * Implements _webform_display_component().
 */
function _webform_display_civicrm_contact($component, $value, $format = 'html') {
  return array(
    '#title' => $component['name'],
    '#weight' => $component['weight'],
    '#theme' => 'webform_display_textfield',
    '#theme_wrappers' => $format == 'html' ? array('webform_element') : array('webform_element_text'),
    '#field_prefix' => '',
    '#field_suffix' => '',
    '#format' => $format,
    '#value' => empty($value[0]) ? '' : webform_civicrm_display_name($value[0]),
    '#translatable' => array('title'),
  );
}

/**
 * Implements _webform_table_component().
 */
function _webform_table_civicrm_contact($component, $value) {
  return empty($value[0]) ? '' : check_plain(webform_civicrm_display_name($value[0]));
}

/**
 * Implements _webform_csv_headers_component().
 */
function _webform_csv_headers_civicrm_contact($component, $export_options) {
  $header = array();
  $header[0] = '';
  $header[1] = '';
  $header[2] = $component['name'];
  return $header;
}

/**
 * Implements _webform_csv_data_component().
 */
function _webform_csv_data_civicrm_contact($component, $export_options, $value) {
  return empty($value[0]) ? '' : webform_civicrm_display_name($value[0]);
}

/**
 * Returns a list of contacts based on component settings.
 */
function webform_civicrm_existing_contact_search($node, $component, $params, $str = NULL) {
  if (empty($node->webform_civicrm)) {
    return array();
  }
  list(, $c, ) = explode('_', $component['form_key'], 3);
  $contact_type = $node->webform_civicrm['data']['contact'][$c]['contact'][1]['contact_type'];
  $limit = $str ? 12 : 200;
  $ret = array();
  $params += array(
    'contact_type' => $contact_type,
    'rowCount' => $limit,
    'sort' => 'sort_name',
    'return.display_name' => TRUE,
  );
  if ($str) {
    require_once 'CRM/Utils/Type.php';
    $params['display_name'] = str_replace(' ', '%', CRM_Utils_Type::escape($str, 'String'));
  }
  $result = webform_civicrm_api('contact', 'get', $params);
  // Autocomplete results
  if ($str) {
    foreach (webform_civicrm_aval($result, 'values', array()) as $val) {
      if (!empty($val['display_name'])) {
        $ret[] = array('id' => $val['id'], 'name' => $val['display_name']);
      }
    }
    if (count($ret) < $limit && $component['extra']['allow_create']) {
      $ret[] = array('id' => "-$str", 'name' => '<em><i>' . $component['extra']['none_prompt'] . '</i></em>');
    }
  }
  // Select results
  else {
    foreach (webform_civicrm_aval($result, 'values', array()) as $val) {
      if (!empty($val['display_name'])) {
        $ret[$val['id']] = $val['display_name'];
      }
    }
    if ($component['extra']['allow_create']) {
      $ret['-'] = $component['extra']['none_prompt'];
    }
  }
  return $ret;
}

/**
 * Callback to serve AJAX requests.
 */
function webform_civicrm_js_options($key, $input = '') {
  civicrm_initialize();
  // Populate state/prov lists on the fly
  if ($key == 'state_province') {
    if (!$input || (intval($input) != $input && $input != 'default')) {
      exit(drupal_json_output(array('' => t('- first choose a country -'))));
    }
    exit(drupal_json_output(webform_civicrm_get_options('state_province', $input)));
  }
  // Populate contact autocomplete
  elseif (strpos($key, '-')) {
    if (empty($_GET['str']) && (empty($_GET['load']) || empty($_GET['cid']))) {
      exit();
    }
    list($nid, $fid) = explode('-', $key, 2);
    $node = node_load($nid);
    if (!webform_civicrm_autocomplete_access($node, $fid)) {
      return drupal_access_denied();
    }
    $component = $node->webform['components'][$fid];
    $filters = webform_civicrm_search_filters($component);
    // Bypass filters when choosing contact for hidden field
    if (!empty($_GET['admin']) && webform_civicrm_admin_access($node)) {
      $filters = array('check_permissions' => TRUE);
      $component['extra']['allow_create'] = 0;
    }
    // Autocomplete contact names
    if (!empty($_GET['str'])) {
      exit(drupal_json_output(webform_civicrm_existing_contact_search($node, $component, $filters, $_GET['str'])));
    }
    // Load contact by id
    $data = array();
    if ($name = webform_civicrm_existing_contact_access($node, $component, $filters, $_GET['cid'])) {
      if ($_GET['load'] == 'name') {
        if ($_GET['cid'][0] === '-') {
          $data = '<em><i>' . $component['extra']['none_prompt'] . '</i></em>';
        }
        else {
          $data = $name;
        }
      }
      // Fetch entire contact to populate form via ajax
      if ($_GET['load'] == 'full') {
        module_load_include('inc', 'webform_civicrm', 'webform_civicrm_forms');
        $enabled = webform_civicrm_enabled_fields($node);
        list(, $c, ) = explode('_', $component['form_key'], 3);
        $contact = webform_civicrm_contact_get($node, $enabled, $c, $_GET['cid'], $component['extra']['hide_fields']);
        // Flatten data into simple form keys & values
        foreach ($enabled as $fid => $f) {
          list(, $i, $ent, $n, $table, $field) = explode('_', $fid, 6);
          if ($i == $c && $ent == 'contact' && isset($contact[$table][$n][$field])) {
            $type = ($table == 'contact' && strpos($field, 'name')) ? 'name' : $table;
            // Exclude blank and hidden fields to save bandwidth
            if ($contact[$table][$n][$field] !== '' && $contact[$table][$n][$field] !== array() && !in_array($type, $component['extra']['hide_fields'])) {
              $data[str_replace('_', '-', $fid)] = $contact[$table][$n][$field];
            }
          }
        }
      }
    }
    exit(drupal_json_output($data));
  }
  return drupal_access_denied();
}

/**
 * Access callback. Check if user has permission to view autocomplete results.
 */
function webform_civicrm_autocomplete_access($node, $fid) {
  global $user;
  if (!$fid || empty($node->webform['components'][$fid]) || !node_access('view', $node)) {
    return FALSE;
  }
  if ($user->uid === 1 || webform_results_access($node)) {
    return TRUE;
  }
  if (!empty($node->webform['components'][$fid]['private'])) {
    return FALSE;
  }
  if (variable_get('webform_submission_access_control', 1)) {
    foreach ($node->webform['roles'] as $rid) {
      $allowed_roles[$rid] = isset($user->roles[$rid]) ? TRUE : FALSE;
    }
    if (array_search(TRUE, $allowed_roles) === FALSE) {
      return FALSE;
    }
  }
  return TRUE;
}

/**
 * Access callback. Check if user should be allowed to load a contact.
 */
function webform_civicrm_existing_contact_access($node, $component, $filters, $cid) {
  // Create new contact doesn't require lookup
  if ($cid[0] === '-' && $component['extra']['allow_create']) {
    return TRUE;
  }
  if (!is_numeric($cid)) {
    return FALSE;
  }
  // First fetch contact name
  $result = webform_civicrm_api('contact', 'get', array('id' => $cid, 'return.display_name' => TRUE));
  if ($name = webform_civicrm_aval($result, 'values:' . $cid . ':display_name')) {
    // Then run it through contact search with the same filters as the user has
    $filters['display_name'] = '"' . $name . '"';
    $filters['return.display_name'] = TRUE;
    list(, $c, ) = explode('_', $component['form_key'], 3);
    $filters['contact_type'] = $node->webform_civicrm['data']['contact'][$c]['contact'][1]['contact_type'];
    $result = webform_civicrm_api('contact', 'get', $filters);
    return isset($result['values'][$cid]) ? $name : FALSE;
  }
  return FALSE;
}

/**
 * Find exposed field sets for a contact
 */
function webform_civicrm_contact_fields($node, $con) {
  $ret = array();
  $sets = webform_civicrm_get_fields('sets');
  $sets['name'] = array('label' => t('Name'));
  foreach ($node->webform['components'] as $f) {
    if ($pieces = webform_civicrm_explode_key($f['form_key'])) {
      list( , $c, , , $table, $field) = $pieces;
      if ($c == $con && isset($sets[$table])) {
        // Separate name from other contact fields
        if ($table == 'contact' && strpos($field, 'name')) {
          $table = 'name';
        }
        if ($field != 'existing') {
          $ret[$table] = $sets[$table]['label'];
        }
      }
    }
  }
  return $ret;
}

/**
 * Format filters for the contact get api
 */
function webform_civicrm_search_filters($component) {
  $params = array();
  foreach ($component['extra']['filters'] as $key => $val) {
    if (!empty($val)) {
      if ($key === 'tag' || $key === 'group') {
        $val = array_fill_keys($val, 1);
      }
      $params[$key] = $val;
    }
  }
  return $params;
}