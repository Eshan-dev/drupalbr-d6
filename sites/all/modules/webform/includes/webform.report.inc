<?php

/**
 * @file
 * This file includes helper functions for creating reports for webform.module
 *
 * @author Nathan Haug <nate@lullabot.com>
 */

// All functions within this file need the webform.submissions.inc.
module_load_include('inc', 'webform', 'includes/webform.submissions');

/**
 * Retrieve lists of submissions for a given webform.
 */
function webform_results_submissions($node, $user_filter, $pager_count) {
  global $user;

  if (isset($_GET['results']) && is_numeric($_GET['results'])) {
    $pager_count = $_GET['results'];
  }

  $header = theme('webform_results_submissions_header', $node);
  if ($user_filter) {
    if ($user->uid) {
      drupal_set_title(t('Submissions for %user', array('%user' => $user->name)));
    }
    else {
      drupal_set_title(t('Your submissions'));
      webform_disable_page_cache();
    }
    webform_set_breadcrumb($node);
    $submissions = webform_get_submissions(array('nid' => $node->nid, 'uid' => $user->uid), $header, $pager_count);
    $count = webform_get_submission_count($node->nid, $user->uid);
  }
  else {
    $submissions = webform_get_submissions($node->nid, $header, $pager_count);
    $count = webform_get_submission_count($node->nid);
  }

  $operation_column = end($header);
  $operation_total = $operation_column['colspan'];

  $rows = array();
  foreach ($submissions as $sid => $submission) {
    $row = array(
      $submission->is_draft ? t('@sid (draft)', array('@sid' => $sid)) : $sid,
      format_date($submission->submitted, 'small'),
    );
    if (webform_results_access($node, $user)) {
      $row[] = theme('username', $submission);
      $row[] = $submission->remote_addr;
    }
    $row[] = l(t('View'), "node/$node->nid/submission/$sid");
    $operation_count = 1;
    // No need to call this multiple times, just reference this in a variable.
    $destination = drupal_get_destination();
    if (webform_submission_access($node, $submission, 'edit', $user)) {
      $row[] = l(t('Edit'), "node/$node->nid/submission/$sid/edit", array('query' => $destination));
      $operation_count++;
    }
    if (webform_submission_access($node, $submission, 'delete', $user)) {
      $row[] = l(t('Delete'), "node/$node->nid/submission/$sid/delete", array('query' => $destination));
      $operation_count++;
    }
    if ($operation_count < $operation_total) {
      $row[count($row) - 1] = array('data' => $row[count($row) - 1], 'colspan' => $operation_total - $operation_count + 1);
    }
    $rows[] = $row;
  }

  $element['#theme'] = 'webform_results_submissions';
  $element['#node'] = $node;
  $element['#submissions'] = $submissions;
  $element['#total_count'] = $count;
  $element['#pager_count'] = $pager_count;

  $element['table']['#theme'] = 'table';
  $element['table']['#header'] = $header;
  $element['table']['#rows'] = $rows;
  $element['table']['#operation_total'] = $operation_total;

  return drupal_render($element);
}

/**
 * Theme the list of links for selecting the number of results per page.
 *
 * @param $total_count
 *   The total number of results available.
 * @param $pager_count
 *   The current number of results displayed per page.
 */
function theme_webform_results_per_page($total_count, $pager_count) {
  $output = '';

  // Create a list of results-per-page options.
  $counts = array(
    '20' => '20',
    '50' => '50',
    '100' => '100',
    '200' => '200',
    '500' => '500',
    '1000' => '1000',
    '0' => t('All'),
  );

  $count_links = array();

  foreach ($counts as $number => $text) {
    if ($number < $total_count) {
      $count_links[] = l($text, $_GET['q'], array('query' => 'results=' . $number, 'attributes' => array('class' => $pager_count == $number ? 'selected' : '')));
    }
  }

  $output .= '<div class="webform-results-per-page">';
  if (count($count_links) > 1) {
    $output .= t('Show !count results per page.', array('!count' => implode(' | ', $count_links)));
  }
  else {
    $output .= t('Showing all results.');
  }
  if ($total_count > 1) {
    $output .= ' ' . t('@total results total.', array('@total' => $total_count));
  }
  $output .= '</div>';

  return $output;
}

/**
 * Theme the header of the submissions table.
 *
 * This is done in it's own function so that webform can retrieve the header and
 * use it for sorting the results.
 */
function theme_webform_results_submissions_header($node) {
  $columns = array(
    array('data' => t('#'), 'field' => 'sid', 'sort' => 'desc'),
    array('data' => t('Submitted'), 'field' => 'submitted'),
  );
  if (webform_results_access($node)) {
    $columns[] = array('data' => t('User'), 'field' => 'name');
    $columns[] = array('data' => t('IP Address'), 'field' => 'remote_addr');
  }
  $columns[] = array('data' => t('Operations'), 'colspan' => module_exists('print') ? 5 : 3);

  return $columns;
}

/**
 * Preprocess function for webform-results-submissions.tpl.php
 */
function template_preprocess_webform_results_submissions(&$vars) {
  $vars['node'] = $vars['element']['#node'];
  $vars['submissions'] = $vars['element']['#submissions'];
  $vars['table'] = $vars['element']['table'];
  $vars['total_count'] = $vars['element']['#total_count'];
  $vars['pager_count'] = $vars['element']['#pager_count'];
  $vars['is_submissions'] = (arg(2) == 'submissions')? 1 : 0;

  unset($vars['element']);
}

/**
 * Create a table containing all submitted values for a webform node.
 */
function webform_results_table($node, $pager_count = 0) {
  if (isset($_GET['results']) && is_numeric($_GET['results'])) {
    $pager_count = $_GET['results'];
  }

  // Get all the submissions for the node.
  $header = theme('webform_results_table_header', $node);
  $submissions = webform_get_submissions($node->nid, $header, $pager_count);
  $total_count = webform_get_submission_count($node->nid);

  $output = theme('webform_results_table', $node, $node->webform['components'], $submissions, $total_count, $pager_count);
  if ($pager_count) {
    $output .= theme('pager', NULL, $pager_count, 0);
  }
  return $output;
}

function theme_webform_results_table_header($node) {
  return array(
    array('data' => t('#'), 'field' => 'sid', 'sort' => 'desc'),
    array('data' => t('Submitted'), 'field' => 'submitted'),
    array('data' => t('User'), 'field' => 'name'),
    array('data' => t('IP Address'), 'field' => 'remote_addr'),
  );
}

/**
 * Theme the results table displaying all the submissions for a particular node.
 *
 * @param $node
 *   The node whose results are being displayed.
 * @param $components
 *   An associative array of the components for this webform.
 * @param $submissions
 *   An array of all submissions for this webform.
 * @param $total_count
 *   The total number of submissions to this webform.
 * @param $pager_count
 *   The number of results to be shown per page.
 */
function theme_webform_results_table($node, $components, $submissions, $total_count, $pager_count) {
  drupal_add_css(drupal_get_path('module', 'webform') . '/css/webform-admin.css', 'theme', 'all', FALSE);

  $header = array();
  $rows = array();
  $cell = array();

  // This header has to be generated seperately so we can add the SQL necessary.
  // to sort the results.
  $header = theme('webform_results_table_header', $node);

  // Generate a row for each submission.
  foreach ($submissions as $sid => $submission) {
    $cell[] = l($sid, 'node/' . $node->nid . '/submission/' . $sid);
    $cell[] = format_date($submission->submitted, 'small');
    $cell[] = theme('username', $submission);
    $cell[] = $submission->remote_addr;
    $component_headers = array();

    // Generate a cell for each component.
    foreach ($node->webform['components'] as $component) {
      $data = isset($submission->data[$component['cid']]['value']) ? $submission->data[$component['cid']]['value'] : NULL;
      $submission_output = webform_component_invoke($component['type'], 'table', $component, $data);
      if ($submission_output !== NULL) {
        $component_headers[] = check_plain($component['name']);
        $cell[] = $submission_output;
      }
    }

    $rows[] = $cell;
    unset($cell);
  }
  if (!empty($component_headers)) {
    $header = array_merge($header, $component_headers);
  }

  if (count($rows) == 0) {
    $rows[] = array(array('data' => t('There are no submissions for this form. <a href="!url">View this form</a>.', array('!url' => url('node/' . $node->nid))), 'colspan' => 4));
  }


  $output = '';
  $output .= theme('webform_results_per_page', $total_count, $pager_count);
  $output .= theme('table', $header, $rows);
  return $output;
}

/**
 * Delete all submissions for a node.
 *
 * @param $nid
 *   The node id whose submissions will be deleted.
 */
function webform_results_clear($nid) {
  $node = node_load($nid);
  $submissions = webform_get_submissions($nid);
  foreach ($submissions as $submission) {
    webform_submission_delete($node, $submission);
  }
}

/**
 * Confirmation form to delete all submissions for a node.
 *
 * @param $nid
 *   ID of node for which to clear submissions.
 */
function webform_results_clear_form($form_state, $node) {
  drupal_set_title(t('Clear Form Submissions'));

  $form = array();
  $form['nid'] = array('#type' => 'value', '#value' => $node->nid);
  $question = t('Are you sure you want to delete all submissions for this form?');

  return confirm_form($form, $question, 'node/' . $node->nid . '/webform-results', NULL, t('Clear'), t('Cancel'));
}

function webform_results_clear_form_submit($form, &$form_state) {
  webform_results_clear($form_state['values']['nid']);
  $node = node_load($form_state['values']['nid']);
  $title = $node->title;

  $message = t('Webform %title entries cleared.', array('%title' => $title));
  drupal_set_message($message);
  watchdog('webform', $message);
  $form_state['redirect'] = 'node/' . $form_state['values']['nid'] . '/webform-results';
}

/**
 * Form to configure the download of CSV files.
 */
function webform_results_download_form(&$form_state, $node) {
  module_load_include('inc', 'webform', 'includes/webform.export');
  module_load_include('inc', 'webform', 'includes/webform.components');

  $form = array();

  $form['node'] = array(
    '#type' => 'value',
    '#value' => $node,
  );

  $form['format'] = array(
    '#type' => 'radios',
    '#title' => t('Export format'),
    '#options' => webform_export_list(),
    '#default_value' => isset($form_state['values']['format']) ? $form_state['values']['format'] : variable_get('webform_export_format', 'delimited'),
  );

  $form['delimiter'] = array(
    '#type' => 'select',
    '#title' => t('Delimited text format'),
    '#description' => t('This is the delimiter used in the CSV/TSV file when downloading Webform results. Using tabs in the export is the most reliable method for preserving non-latin characters. You may want to change this to another character depending on the program with which you anticipate importing results.'),
    '#default_value' => isset($form_state['values']['delimiter']) ? $form_state['values']['delimiter'] : variable_get('webform_csv_delimiter', '\t'),
    '#options' => array(
      ','  => t('Comma (,)'),
      '\t' => t('Tab (\t)'),
      ';'  => t('Semicolon (;)'),
      ':'  => t('Colon (:)'),
      '|'  => t('Pipe (|)'),
      '.'  => t('Period (.)'),
      ' '  => t('Space ( )'),
    ),
  );

  $form['select_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select list options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['select_options']['select_keys'] = array(
    '#type' => 'radios',
    '#title' => t('Select keys'),
    '#options' => array(
      0 => t('Full, human-readable options (values)'),
      1 => t('Short, raw options (keys)'),
    ),
    '#default_value' => isset($form_state['values']['select_options']['select_keys']) ? $form_state['values']['select_options']['select_keys'] : 0,
    '#description' => t('Choose which part of options should be displayed from key|value pairs.'),
  );

  $form['select_options']['select_format'] = array(
    '#type' => 'radios',
    '#title' => t('Select list format'),
    '#options' => array(
      'separate' => t('Separate'),
      'compact' => t('Compact'),
    ),
    '#default_value' => isset($form_state['values']['select_options']['select_format']) ? $form_state['values']['select_options']['select_format'] : 'separate',
    '#attributes' => array('class' => 'webform-select-list-format'),
    '#theme' => 'webform_results_download_select_format',
  );

  $csv_components = array(
    'info' => t('Submission information'),
    'serial' => '-' . t('Submission Number'),
    'sid' => '-' . t('Submission ID'),
    'time' => '-' . t('Time'),
    'draft' => '-' . t('Draft'),
    'ip_address' => '-' . t('IP Address'),
    'uid' => '-' . t('User ID'),
    'username' => '-' . t('Username'),
  );
  $csv_components += webform_component_list($node, 'csv', TRUE);

  $form['components'] = array(
    '#type' => 'select',
    '#title' => t('Included export components'),
    '#options' => $csv_components,
    '#default_value' => isset($form_state['values']['components']) ? $form_state['values']['components'] : array_keys($csv_components),
    '#multiple' => TRUE,
    '#size' => 10,
    '#description' => t('The selected components will be included in the export.'),
    '#process' => array('webform_component_select'),
  );

  $form['range'] = array(
    '#type' => 'fieldset',
    '#title' => t('Download range options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
    '#theme' => 'webform_results_download_range',
    '#element_validate' => array('webform_results_download_range_validate'),
    '#after_build' => array('webform_results_download_range_after_build'),
  );

  $form['range']['range_type'] = array(
    '#type' => 'radios',
    '#options' => array(
      'all' => t('All submissions'),
      'new' => t('Only new submissions since your last download'),
      'latest' => t('Only the latest'),
      'range' => t('All submissions starting from'),
    ),
    '#default_value' => 'all',
  );
  $form['range']['latest'] = array(
    '#type' => 'textfield',
    '#size' => 5,
    '#maxlength' => 8,
    '#default_value' => '',
  );
  $form['range']['start'] = array(
    '#type' => 'textfield',
    '#size' => 5,
    '#maxlength' => 8,
    '#default_value' => '',
  );
  $form['range']['end'] = array(
    '#type' => 'textfield',
    '#size' => 5,
    '#maxlength' => 8,
    '#default_value' => '',
    '#description' => '',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Download'),
  );

  return $form;
}

/**
 * FormAPI element validate function for the range fieldset.
 */
function webform_results_download_range_validate($element, $form_state) {
  switch ($element['range_type']['#value']) {
    case 'latest':
      // Download latest x submissions.
      if ($element['latest']['#value'] == '') {
        form_error($element['latest'], t('Latest number of submissions field is required.'));
      }
      else{
        if (!is_numeric($element['latest']['#value'])) {
          form_error($element['latest'], t('Latest number of submissions must be numeric.'));
        }
        else{
          if ($element['latest']['#value'] <= 0) {
            form_error($element['latest'], t('Latest number of submissions must be greater than 0.'));
          }
        }
      }
      break;
    case 'range':
      // Download Start-End range of submissions.
      // Start submission number.
      if ($element['start']['#value'] == '') {
        form_error($element['start'], t('Start submission number is required.'));
      }
      else{
        if (!is_numeric($element['start']['#value'])) {
          form_error($element['start'], t('Start submission number must be numeric.'));
        }
        else{
          if ($element['start']['#value'] <= 0) {
            form_error($element['start'], t('Start submission number must be greater than 0.'));
          }
        }
      }
      // End submission number.
      if ($element['end']['#value'] != '') {
        if (!is_numeric($element['end']['#value'])) {
          form_error($element['end'], t('End submission number must be numeric.'));
        }
        else{
          if ($element['end']['#value'] <= 0) {
            form_error($element['end'], t('End submission number must be greater than 0.'));
          }
          else{
            if ($element['end']['#value'] < $element['start']['#value']) {
              form_error($element['end'], t('End submission number may not be less than Start submission number.'));
            }
          }
        }
      }
      break;
  }

}

/**
 * Validate handler for webform_results_download_form().
 */
function webform_results_download_form_submit(&$form, &$form_state) {
  $options = array(
    'delimiter' => $form_state['values']['delimiter'],
    'components' => array_keys(array_filter($form_state['values']['components'])),
    'select_keys' => $form_state['values']['select_keys'],
    'select_format' => $form_state['values']['select_format'],
    'range_type' => $form_state['values']['range']['range_type'],
  );

  // Retrieve the list of required SIDs.
  if ($options['range_type'] != 'all') {
    $options['sids'] = webform_download_sids($form_state['values']['node']->nid, $form_state['values']['range']);
  }

  webform_results_download($form_state['values']['node'], $form_state['values']['format'], $options);
}

/**
 * FormAPI after build function for the download range fieldset.
 */
function webform_results_download_range_after_build($element, &$form_state) {
  $node = $form_state['values']['node'];

  // Build a list of counts of new and total submissions.
  $count = webform_get_submission_count($node->nid);
  $sids = webform_download_sids($node->nid, array('range_type' => 'new'));

  $last_download = webform_download_last_download_info($node->nid);

  $element['#webform_download_info']['sid'] = $last_download ? $last_download['sid'] : 0;
  $element['#webform_download_info']['requested'] = $last_download ? $last_download['requested'] : $node->created;
  $element['#webform_download_info']['total'] = $count;
  $element['#webform_download_info']['new'] = count($sids);

  return $element;
}

/**
 * Theme the output of the export range fieldset.
 */
function theme_webform_results_download_range($element) {
  drupal_add_css(drupal_get_path('module', 'webform') . '/css/webform-admin.css', 'theme', 'all', FALSE);
  drupal_add_js(drupal_get_path('module', 'webform') . '/js/webform-admin.js', 'module', 'header', FALSE, TRUE, FALSE);

  $download_info = $element['#webform_download_info'];

  // Set description for total of all submissions.
  $element['range_type']['all']['#description'] = '(' . t('@count total', array('@count' => $download_info['total'])) . ')';
  $element['range_type']['all']['#prefix'] = '<div class="webform-container-inline">';
  $element['range_type']['all']['#suffix'] = '</div>';

  // Set description for "New submissions since last download".
  $format = webform_date_format('short');
  $requested_date = format_date($download_info['requested'], 'custom', $format);
  $element['range_type']['new']['#description'] = '(' . t('@count new since @date', array('@count' => $download_info['new'], '@date' => $requested_date)) . ')';
  $element['range_type']['new']['#prefix'] = '<div class="webform-container-inline">';
  $element['range_type']['new']['#suffix'] = '</div>';

  // Disable option if there are no new submissions.
  if ($download_info['new'] == 0) {
    $element['range_type']['new']['#attributes']['disabled'] = 'disabled';
  }

  // Render latest x submissions option.
  $element['latest']['#attributes']['class'] = 'webform-set-active';
  $element['range_type']['latest']['#title'] = t('Only the latest !number submissions', array('!number' => drupal_render($element['latest'])));
  $element['range_type']['latest']['#prefix'] = '<div class="webform-container-inline">';
  $element['range_type']['latest']['#suffix'] = '</div>';

  // Render Start-End submissions option.
  $element['start']['#attributes']['class'] = 'webform-set-active';
  $element['end']['#attributes']['class'] = 'webform-set-active';
  $element['range_type']['range']['#title'] = t('All submissions starting from: !start and optionally to: !end', array('!start' => drupal_render($element['start']), '!end' => drupal_render($element['end'])));
  $element['range_type']['range']['#prefix'] = '<div class="webform-container-inline">';
  $element['range_type']['range']['#suffix'] = '</div>';


  $last_sid = $download_info['sid'] ? $download_info['sid'] : theme('placeholder', t('none'));
  $element['range_type']['range']['#description'] = '(' . t('Use submission IDs for the range. Last downloaded end SID: !sid.', array('!sid' => $last_sid)) . ')';

  return drupal_render($element);
}

/**
 * Theme the output of the select list format radio buttons.
 */
function theme_webform_results_download_select_format($element) {
  drupal_add_css(drupal_get_path('module', 'webform') . '/css/webform-admin.css', 'theme', 'all', FALSE);

  $output = '';

  // Build an example table for the separate option.
  $header = array(t('Option A'), t('Option B'), t('Option C'));
  $rows = array(
    array('X', '', ''),
    array('X', '', 'X'),
    array('', 'X', 'X'),
  );

  $element['separate']['#attributes']['class'] = '';
  $element['separate']['#description'] = theme('table', $header, $rows);
  $element['separate']['#description'] .= t('Separate options are more suitable for building reports, graphs, and statistics in a spreadsheet application.');
  $output .= drupal_render($element['separate']);

  // Build an example table for the compact option.
  $header = array(t('My select list'));
  $rows = array(
    array('Option A'),
    array('Option A,Option C'),
    array('Option B,Option C'),
  );

  $element['separate']['#attributes']['class'] = '';
  $element['compact']['#description'] = theme('table', $header, $rows);
  $element['compact']['#description'] .= t('Compact options are more suitable for importing data into other systems.');
  $output .= drupal_render($element['compact']);

  return $output;
}

/**
 * Generate a Excel-readable CSV file containing all submissions for a Webform.
 *
 * The CSV requires that the data be presented in a flat file.  In order
 * to maximize usability to the Excel community and minimize subsequent
 * stats or spreadsheet programming this program extracts data from the
 * various records for a given session and presents them as a single file
 * where each row represents a single record.
 * The structure of the file is:
 *   Heading Line 1: Gives group overviews padded by empty cells to the
 *                   next group.  A group may be a question and corresponds
 *                   to a component in the webform philosophy. Each group
 *                   overview will have a fixed number of columns beneath it.
 *   Heading line 2: gives column headings
 *   Data line 1 .....
 *   Data line 2 .....
 *
 * An example of this format is given below.  Note the columns have had spaces
 * added so the columns line up.  This is not the case with actual file where
 * a column may be null.  Note also, that multiple choice questions as produced
 * by checkboxes or radio buttons have been presented as "yes" or "no" and the
 * actual choice text is retained only in the header line 2.
 * Data from text boxes and input fields are written out in the body of the table.
 *
 *   Submission Details,    ,   ,      ,Question 1,        ,        ,..,        ,Question 2,        ,        ,..,        ,Question n
 *   timestamp         ,time,SID,userid,Choice 1  ,Choice 2,Choice 3,..,Choice n,Choice 1  ,Choice 2,Choice 3,..,Choice n,Comment
 *   21 Feb 2005       ,1835,23 ,34    ,X         ,        ,        ,..,       ,X          ,X       ,X       ,..,X       ,My comment
 *   23 Feb 2005       ,1125,24 ,89    ,X         ,X       ,        ,..,       ,X          ,X       ,X       ,..,X       ,Hello
 *   .................................................................................................................................
 *   27 Feb 2005       ,1035,56 ,212   ,X         ,        ,        ,..,       ,X          ,X       ,X       ,..,X       ,How is this?
 *
 */
function webform_results_download($node, $format = 'delimited', $options = array()) {
  global $user;
  module_load_include('inc', 'webform', 'includes/webform.export');
  module_load_include('inc', 'webform', 'includes/webform.components');

  $submission_information = array(
    'serial' => t('Serial'),
    'sid' => t('SID'),
    'time' => t('Time'),
    'draft' => t('Draft'),
    'ip_address' => t('IP Address'),
    'uid' => t('UID'),
    'username' => t('Username'),
  );

  if (empty($options)) {
    $options = array(
      'delimiter' => variable_get('webform_csv_delimiter', '\t'),
      'components' => array_keys($submission_information) + array_keys(webform_component_list($node, 'csv', TRUE)),
      'components' => array_merge(array_keys($submission_information), array_keys(webform_component_list($node, 'csv', TRUE))),
      'select_display' => 'value',
      'select_format' => 'separate',
      'range_type' => 'all',
    );
  }
  else {
    foreach ($submission_information as $key => $label) {
      if (!in_array($key, $options['components'])) {
        unset($submission_information[$key]);
      }
    }
  }

  // Open a new Webform exporter object.
  $exporter = webform_export_create_handler($format, $options);

  $file_name = tempnam(variable_get('file_directory_temp', file_directory_temp()), 'webform');
  $handle = @fopen($file_name, 'w'); // The @ suppresses errors.
  $exporter->bof($handle);

  // Fill in the header for the submission information (if any).
  $header[2] = $header[1] = $header[0] = count($submission_information) ? array_fill(0, count($submission_information), '') : array();
  if (count($submission_information)) {
    $header[0][0] = $node->title;
    $header[1][0] = t('Submission Details');
    foreach (array_values($submission_information) as $column => $label) {
      $header[2][$column] = $label;
    }
  }

  // Compile header information for components.
  foreach ($options['components'] as $cid) {
    if (isset($node->webform['components'][$cid])) {
      $component = $node->webform['components'][$cid];

      // Let each component determine its headers.
      if (webform_component_feature($component['type'], 'csv')) {
        $component_header = (array) webform_component_invoke($component['type'], 'csv_headers', $component, $options);
        $header[0] = array_merge($header[0], (array) $component_header[0]);
        $header[1] = array_merge($header[1], (array) $component_header[1]);
        $header[2] = array_merge($header[2], (array) $component_header[2]);
      }
    }
  }

  // Add headers to the file.
  foreach ($header as $row) {
    $exporter->add_row($handle, $row);
  }

  // Get all the submissions for the node.
  $filters['nid'] = $node->nid;
  if (!empty($options['sids'])){
    $filters['sid'] = $options['sids'];
  }
  $submissions = webform_get_submissions($filters);

  // Generate a row for each submission.
  $row_count = 0;
  foreach ($submissions as $sid => $submission) {
    $row_count++;

    $row = array();
    if (isset($submission_information['serial'])) {
      $row[] = $row_count;
    }
    if (isset($submission_information['sid'])) {
      $row[] = $sid;
    }
    if (isset($submission_information['time'])) {
      $row[] = format_date($submission->submitted, 'small');
    }
    if (isset($submission_information['draft'])) {
      $row[] = $submission->is_draft;
    }
    if (isset($submission_information['ip_address'])) {
      $row[] =  $submission->remote_addr;
    }
    if (isset($submission_information['uid'])) {
      $row[] = $submission->uid;
    }
    if (isset($submission_information['username'])) {
      $row[] = $submission->name;
    }

    foreach ($options['components'] as $cid) {
      if (isset($node->webform['components'][$cid])) {
        $component = $node->webform['components'][$cid];
        // Let each component add its data.
        $raw_data = isset($submission->data[$cid]['value']) ? $submission->data[$cid]['value'] : NULL;
        if (webform_component_feature($component['type'], 'csv')) {
          $data = webform_component_invoke($component['type'], 'csv_data', $component, $options, $raw_data);
          if (is_array($data)) {
            $row = array_merge($row, array_values($data));
          }
          else {
            $row[] = isset($data) ? $data : '';
          }
        }
      }
    }

    // Write data from submissions.
    $data = $exporter->add_row($handle, $row);
  }

  // Add the closing bytes.
  $exporter->eof($handle);

  // Close the file.
  @fclose($handle);

  $export_name = _webform_safe_name($node->title);
  $exporter->set_headers($export_name);
  @readfile($file_name);  // The @ makes it silent.
  @unlink($file_name);  // Clean up, the @ makes it silent.

  // Update user last downloaded sid if required.
  if ($options['range_type'] != 'range' && $row_count > 0) {
    // Delete existing record.
    db_query('DELETE FROM {webform_last_download} WHERE nid = %d AND uid = %d', $node->nid, $user->uid);
    // Write new record.
    db_query('INSERT INTO {webform_last_download} (nid, uid, sid, requested) VALUES (%d, %d, %d, %d)', $node->nid, $user->uid, $sid, time());
  }

  exit();
}

/**
 * Provides a simple analysis of all submissions to a webform.
 *
 * @param $node
 *   The webform node on which to generate the analysis.
 * @param $sids
 *   An array of submission IDs to which this analysis may be filtered. May be
 *   used to generate results that are per-user or other groups of submissions.
 * @param $analysis_component
 *   A webform component. If passed in, additional information may be returned
 *   relating specifically to that component's analysis, such as a list of
 *   "Other" values within a select list.
 */
function webform_results_analysis($node, $sids = array(), $analysis_component = NULL) {
  if (!is_array($sids)) {
    $sids = array();
  }

  // If showing a component's details, we don't want to loose the menu tabs.
  if ($analysis_component) {
    $item = menu_get_item('node/' . $node->nid . '/webform-results/analysis');
    menu_set_item(NULL, $item);
  }

  $components = isset($analysis_component) ? array($analysis_component['cid'] => $analysis_component) : $node->webform['components'];
  $data = array();
  foreach ($components as $cid => $component) {
    // Do component specific call.
    if ($row_data = webform_component_invoke($component['type'], 'analysis', $component, $sids, isset($analysis_component))) {
      $data[$cid] = $row_data;
    }
  }

  return theme('webform_results_analysis', $node, $data, $sids, $analysis_component);
}

/**
 * Output the content of the Analysis page.
 *
 * @see webform_results_analysis()
 */
function theme_webform_results_analysis($node, $data, $sids = array(), $analysis_component = NULL) {

  $rows = array();
  $question_number = 0;
  $single = isset($analysis_component);

  $header = array(
    $single ? $analysis_component['name'] : t('Q'),
    array('data' => $single ? '&nbsp;' : t('responses'), 'colspan' => '10')
  );

  foreach ($data as $cid => $row_data) {
    $question_number++;

    if (is_array($row_data)) {
      $row = array();
      if (!$single) {
        $row['data'][] = array('data' => '<strong>' . $question_number . '</strong>', 'rowspan' => count($row_data) + 1, 'valign' => 'top');
        $row['data'][] = array('data' => '<strong>' . check_plain($node->webform['components'][$cid]['name']) . '</strong>', 'colspan' => '10');
        $row['class'] = 'webform-results-question';
      }
      $rows = array_merge($rows, array_merge(array($row), $row_data));
    }
  }

  if (count($rows) == 0) {
    $rows[] = array(array('data' => t('There are no submissions for this form. <a href="!url">View this form</a>.', array('!url' => url('node/' . $node->nid))), 'colspan' => 20));
  }

  return theme('table', $header, $rows, array('class' => 'webform-results-analysis'));
}

/**
 * Given a set of range options, retrieve a set of SIDs for a webform node.
 */
function webform_download_sids($nid, $range_options, $uid = NULL) {
  $query = 'SELECT sid FROM {webform_submissions} WHERE nid = %d';
  $args = array($nid);

  switch ($range_options['range_type']) {
    case 'all':
      // All Submissions.
      $query .= ' ORDER BY sid ASC';
      break;
    case 'new':
      // All Since Last Download.
      $download_info = webform_download_last_download_info($nid, $uid);
      $last_sid = $download_info ? $download_info['sid'] : 0;
      $query .= ' AND sid > %d ORDER BY sid ASC';
      $args[] = $last_sid;
      break;
    case 'latest':
      // Last x Submissions.
      $query .= ' ORDER BY sid DESC LIMIT 0, %d';
      $args[] = $range_options['latest'];
      break;
    case 'range':
      // Submissions Start-End.
      $query .=' AND sid >= %d';
      $args[] = $range_options['start'];
      if ($range_options['end']) {
        $query .= ' AND sid <= %d';
        $args[] = $range_options['end'];
      }
      $query .= ' ORDER BY sid ASC';
      break;
  }
  $result = db_query($query, $args);
  $sids = array();
  while ($row = db_fetch_object($result)) {
    $sids[] = $row->sid;
  }

  // The last x submissions option has SIDs that are in reverse order.
  if ($range_options['range_type'] == 'latest') {
    $sids = array_reverse($sids);
  }

  return $sids;
}

/**
 * Get this user's last download information, including the SID and timestamp.
 *
 * This function provides an array of information about the last download that
 * a user had for a particular Webform node. Currently it only returns an array
 * with two keys:
 *  - sid: The last submission ID that was downloaded.
 *  - requested: The timestamp of the last download request.
 *
 * @param $nid
 *   The Webform NID.
 * @param $uid
 *   The user account ID for which to retrieve download information.
 * @return
 *   An array of download information or FALSE if this user has never downloaded
 *   results for this particular node.
 */
function webform_download_last_download_info($nid, $uid = NULL) {
  $uid = isset($uid) ? $uid : $GLOBALS['user']->uid;
  $result = db_query("SELECT * FROM {webform_last_download} WHERE nid = %d AND uid = %d", $nid, $uid);
  $info = db_fetch_array($result);

  return $info;
}
