<?php
/**
 * @file
 * Contains the Calendar row style plugin.
 */

/**
 * Plugin which creates a view on the resulting object
 * and formats it as a Calendar node.
 */
class calendar_plugin_row_node extends views_plugin_row {

  // Basic properties that let the row style follow relationships.
  var $base_table = 'node';
  var $base_field = 'nid';

  // Stores the nodes loaded with pre_render.
  var $nodes = array();

  /**
   * Helper function to find the date argument handler for this view.
   */
  function date_argument_handler() {
    foreach ($this->view->argument as $name => $handler) {
      if (date_views_handler_is_date($handler, 'argument')) {
        return $handler;
      }
    }
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['date_fields'] = array('default' => array());
    return $options;
  }

  /**
   * Provide a form for setting options.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    
    $form['#markup'] = t("The calendar row plugin will format nodes as calendar items. Make sure this display has a 'Calendar' format and uses a 'Date' contextual filter, or this plugin will not work correctly. The 'body' of the calendar item is constructed using the 'Calendar' view mode on the 'Display Fields' screen. Visit that screen to select the fields that should be displayed in the calendar body.");
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

    $data = date_views_fields('node');
    $info = $data['name'][$this->options['date_field']];

    $field_name  = str_replace(array('_value', '_value2'), '', $info['real_field_name']);
    $table_name  = $info['table_name'];
    $delta_field = $info['delta_field'];
    $is_field = !empty($delta_field);

    // Identify the field value that matched our query.
    // @TODO Need to come back after this is all working and make more timezone adjustments.    
        
    $item = $node->$field_name;
    $node->date_id = array();
    $start = NULL;
    $end   = NULL;
    if ($is_field) {
      $lang = $node->language;
      $delta = isset($row->$delta_field) ? $row->$delta_field : 0;
      $item = array_key_exists($lang, $item) ? $item[$lang][$delta] : $item['und'][$delta];
      $node->date_id[] = 'calendar-ical.' . $node->nid . '.' . $field_name . '.' . $delta;

      if (!empty($item['value'])) {
        $start = new dateObject($item['value'], 'UTC');
        $end   = array_key_exists('value2', $item) ? new dateObject($item['value2'], 'UTC') : $start;
      }
    }
    elseif (!$is_field && !empty($item)) {
      $start = new dateObject($item, 'UTC');
      $end   = new dateObject($item, 'UTC');
     }
    
    // If we don't have date value, go no further.
    if (empty($start)) {
      return;
    }
    
    $item_text = '';

    // Create the rendered display using the display settings from the 'Calendar' view mode.
    node_build_content($node, 'calendar');

    if (!empty($node->content)) {
      // We render node contents and force links to be last.
      $links = drupal_render($node->content['links']);
      $item_text .= drupal_render($node->content) . $links;
    }

    $item = new stdClass();
    $item->title = $node->title;
    $item->body = $item_text;
    $item->item_start = $start;
    $item->item_end = $end;
    $item->item = $item;
    
    return theme($this->theme_functions(),
      array(
        'view' => $this->view,
        'options' => $this->options,
        'item' => $item
      ));
  }
}

