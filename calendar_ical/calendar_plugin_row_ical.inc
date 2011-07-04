<?php
/**
 * @file
 * Contains the iCal row style plugin.
 */

/**
 * Plugin which performs a view on the resulting object
 * and formats it as an iCal item.
 */
class calendar_plugin_row_ical extends views_plugin_row {

  // Basic properties that let the row style follow relationships.
  var $base_table = 'node';
  var $base_field = 'nid';

  // Stores the nodes loaded with pre_render.
  var $nodes = array();

  function option_definition() {
    $options = parent::option_definition();

    $options['date_field'] = array('default' => array());
    return $options;
  }

  /**
   * Provide a form for setting options.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $options = $this->display->handler->get_field_labels();

    $options_optional = array('' => '') + $options;

    $form['date_field'] = array(
      '#type' => 'select',
      '#title' => t('Date field'),
      '#options' => $options_optional,
      '#default_value' => $this->options['date_field'],
      '#description' => t('The view field that should be formatted as the iCal date.'),
      '#required' => TRUE,
    );
  }

  function pre_render($values) {
    $nids = array();
    foreach ($values as $row) {
      $nids[] = $row->{$this->field_alias};
    }
    if (!empty($nids)) {
      $this->nodes = node_load_multiple($nids);
    }
  }

  function render($row) {
    // For the most part, this code is taken from node_feed() in node.module
    global $base_url;

    $nid = $row->{$this->field_alias};
    if (!is_numeric($nid)) {
      return;
    }

    // Load the specified node:
    $node = $this->nodes[$nid];
    if (empty($node)) {
      return;
    }

    $item_text = '';

    // Create the rendered display using the display settings from the 'iCal' view mode.
    node_build_content($node, 'ical');

    if ($item_length != 'title' && !empty($node->content)) {
      // We render node contents and force links to be last.
      $links = drupal_render($node->content['links']);
      $item_text .= drupal_render($node->content) . $links;
    }

    $date_field = $this->options['date_field'];

    $rrule_field = $node->raw[$date_field]['rrule'];
    $event = array();
    $event['summary'] = $node->title;
    $event['start'] = $node->raw[$date_field];
    $event['end'] = $node->raw[$date_field];
    $event['description'] = $item_text;
    $event['url'] = url("node/$node->nid", array('absolute' => TRUE));
    $event['uid'] = !empty($node->date_id) ? $node->date_id : $event['url'];
    $event['rrule'] = !empty($rrule_field) && !empty($node->rendered[$rrule_field]) ? $node->rendered[$rrule_field] : '';
    $events[$event['uid']] = $event;

    return theme($this->theme_functions(),
      array(
        'view' => $this->view,
        'options' => $this->options,
        'row' => $event
      ));
  }
}

