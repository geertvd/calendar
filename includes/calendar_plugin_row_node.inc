<?php
/**
 * @file
 * Contains the Calendar row style plugin.
 *
 * This plugin takes the view results, finds the date value for each,
 * then compares that date to the date range for the current view.
 * Items that started before or ended after the current date range
 * are shortened to the current range. Items that extend over more
 * than one day are cloned to create a calendar item for each day.
 * The resulting array of results (which may have a different number
 * of items than the original view result) are then passed back
 * to the style plugin so they can be displayed in a calendar.
 *
 * Row plugins are specific to entity types. To create a row plugin
 * for other types of entities, this class can be extended or copied,
 * adjusting the parts that are specific to nodes.
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
    
    $form['#markup'] = t("The calendar row plugin will format view results as calendar items. Make sure this display has a 'Calendar' format and uses a 'Date' contextual filter, or this plugin will not work correctly.");
  }

  function pre_render($values) {

    // Preload each node used in this view from the cache.
    // Provides all the node values relatively cheaply, and we don't
    // need to do it repeatedly for the same node if there are
    // multiple results for one node.
    $nids = array();
    foreach ($values as $row) {
      // Use the $nid as the key so we don't create more than one value per node.
      $nid = $row->{$this->field_alias};
      $nids[$nid] = $nid;
    }
    if (!empty($nids)) {
      $this->nodes = node_load_multiple($nids);
    }

    // Identify the date argument and fields that apply to this view.
    // Preload the Date Views field info for each field, keyed by the
    // field name, so we know how to retrieve field values from the cached node.
    $data = date_views_fields('node');
    $data = $data['name'];
    $date_fields = array();
    foreach ($this->view->argument as $handler) {
      if (date_views_handler_is_date($handler, 'argument')) {
        // If this is the complex Date argument, the date fields are stored in the handler options, 
        // otherwise we are using the simple date field argument handler.
        if ($handler->definition['handler'] != 'date_views_argument_handler') {
          $alias = $handler->table_alias . '.' . $handler->field;
          $info = $data[$alias];
          $field_name  = str_replace(array('_value', '_value2'), '', $info['real_field_name']);
          $date_fields[$field_name] = $info;
        }
        else {
          foreach ($handler->options['date_fields'] as $alias) {
            $info = $data[$alias];
            $field_name  = str_replace(array('_value', '_value2'), '', $info['real_field_name']);
            $date_fields[$field_name] = $info;
          }
        }
        $this->date_argument = $handler;
        $this->date_fields = $date_fields;
      }
    }
  }

  function render($row) {
    global $base_url;
    $date_info = $this->date_argument->view->date_info;

    $nid = $row->{$this->field_alias};
    if (!is_numeric($nid)) {
      return;
    }

    // Load the specified node:
    $node = $this->nodes[$nid];
    if (empty($node)) {
      return;
    }

    // There could be more than one date field in a view 
    // so iterate through all of them to find the right values
    // for this view result.
    $rows = array();
    foreach ($this->date_fields as $field_name => $info) {

      $table_name  = $info['table_name'];
      $delta_field = $info['delta_field'];
      $tz_handling = $info['tz_handling'];
      $tz_field    = $info['timezone_field'];
      $rrule_field = $info['rrule_field'];
      
      $is_field = !empty($delta_field);
     
      // Retrieve the field value that matched our query from the cached node.
      // Find the date and set it to the right timezone.  
          
      $item = $node->$field_name;
      $node->date_id = array();
      $item_start_date = NULL;
      $item_end_date   = NULL;

      $db_tz   = date_get_timezone_db($tz_handling, isset($item->$tz_field) ? $item->$tz_field : $date_info->display_timezone_name);
      $to_zone = date_get_timezone($tz_handling, isset($item->$tz_field) ? $item->$tz_field : $date_info->display_timezone_name);

      if ($is_field) {

        // @TODO Not sure if this is the correct way to retrieve the right language value, but I think so.
        $lang = $node->language;
        $delta = isset($row->$delta_field) ? $row->$delta_field : 0;
        $item = array_key_exists($lang, $item) ? $item[$lang][$delta] : $item['und'][$delta];

        // Set the date_id for the node, used to identify which field value to display for
        // fields that have multiple values. The theme expects it to be an array.
        $node->date_id = array('calendar.' . $node->nid . '.' . $field_name . '.' . $delta);
     
        if (!empty($item['value'])) {
          $item_start_date = new dateObject($item['value'], $db_tz);
          $item_end_date   = array_key_exists('value2', $item) ? new dateObject($item['value2'], $db_tz) : $item_start_date;
        }
      }
      elseif (!$is_field && !empty($item)) {
        $item_start_date = new dateObject($item, $db_tz);
        $item_end_date   = $item_start_date;
      }
      
      // If we don't have date value, go no further.
      if (empty($item_start_date)) {
        return;
      }

      // Set the item date to the proper display timezone;      
      $item_start_date->setTimezone(new dateTimezone($to_zone));
      $item_end_date->setTimezone(new dateTimezone($to_zone));

      $item_text = '';
     
      // @TODO This displays the node view, need to come back and render the fields instead.

      // Create the rendered display using the display settings from the 'Calendar' view mode.
      node_build_content($node, 'calendar');
     
      if (!empty($node->content)) {
        // We render node contents and force links to be last.
        $links = drupal_render($node->content['links']);
        $item_text .= drupal_render($node->content) . $links;
      }
     
      $event = new stdClass();
      $event->nid = $node->nid;
      $event->title = $node->title;
      // The theme expects a 'type', so populate the $event->type 
      // with some value if used later on entities other than nodes.
      $event->type = $node->type;
      $event->body = $item_text;
      $event->date_start = $item_start_date;
      $event->date_end = $item_end_date;
      $event->date_id = $node->date_id[0];
      $event->class = implode(' ', array($event->date_id, 'calendar', $date_info->granularity . 'view'));
      $event->raw = $item;

      // Now that we have a 'node' for each view result, we need 
      // to remove anything outside the view date range,
      // and possibly create additional nodes so that we have a 'node' 
      // for each day that this item occupies in this view.
      $now = max($date_info->min_zone_string, $item_start_date->format(DATE_FORMAT_DATE));
      $to  = min($date_info->max_zone_string, $item_end_date->format(DATE_FORMAT_DATE));
      $next = new DateObject($now . ' 00:00:00', $date_info->display_timezone);
      if ($date_info->display_timezone_name != $to_zone) {
        // Make $start and $end (derived from $node) use the timezone $to_zone, just as the original dates do.
        date_timezone_set($next, timezone_open($to_zone));
      }
      if (empty($to) || $now > $to) {
        $to = $now;
      }
      // $now and $next are midnight (in display timezone) on the first day where node will occur.
      // $to is midnight on the last day where node will occur.
      // All three were limited by the min-max date range of the view.
      $pos = 0;
      while (!empty($now) && $now <= $to) {
        $node = clone($event);
        $node->url = url('node/' . $node->nid, array('absolute' => TRUE));

        // Get start and end of current day.
        $start = $next->format(DATE_FORMAT_DATETIME);
        date_modify($next, '+1 day');
        date_modify($next, '-1 second');
        $end = $next->format(DATE_FORMAT_DATETIME);

        // Get start and end of item, formatted the same way.
        $item_start = $item_start_date->format(DATE_FORMAT_DATETIME);
        $item_end = $item_end_date->format(DATE_FORMAT_DATETIME);

        // Get intersection of current day and the node value's duration (as strings in $to_zone timezone).
        $node->calendar_start = $item_start < $start ? $start : $item_start;
        $node->calendar_end = !empty($item_end) ? ($item_end > $end ? $end : $item_end) : $node->calendar_start;
        
        // Make date objects
        $node->calendar_start_date = date_create($node->calendar_start, timezone_open($to_zone));
        $node->calendar_end_date = date_create($node->calendar_end, timezone_open($to_zone));

        // Change string timezones into 
        // calendar_start and calendar_end are UTC dates as formatted strings
        $node->calendar_start = date_format($node->calendar_start_date, DATE_FORMAT_DATETIME);
        $node->calendar_end = date_format($node->calendar_end_date, DATE_FORMAT_DATETIME);

        $granularity = 'second';
        $increment = 1;
        if ($is_field) {
          $cck_field = field_info_field($field_name);
          $instance = field_info_instance($this->view->base_table, $field_name, $node->type);
          $granularity = $cck_field['settings']['granularity'];
          $increment = $instance['widget']['settings']['increment'];
        }

        $node->calendar_all_day = date_is_all_day($node->calendar_start, $node->calendar_end, $granularity, $increment);

        // Change string timezones into 
        // calendar_start and calendar_end are UTC dates as formatted strings
        $node->calendar_start = date_format($node->calendar_start_date, DATE_FORMAT_DATETIME);
        $node->calendar_end = date_format($node->calendar_end_date, DATE_FORMAT_DATETIME);

        unset($node->calendar_fields);
        if (isset($node) && (empty($node->calendar_start))) {
          // if no date for the node and no date in the item
          // there is no way to display it on the calendar
          unset($node);
        }
        else {
          // @TODO Add stripes back in later.
          //calendar_node_stripe($this->view, $node, $alias, $alias);
          //calendar_node_taxonomy_stripe($this->view, $node, $alias, $alias);
          //calendar_node_group_stripe($this->view, $node, $alias, $alias);
          $node->date_id .= '.' . $pos;

          $output = theme($this->theme_functions(),
            array(
              'view' => $this->view,
              'options' => $this->options,
              'item' => $node
            ));
          $node->rendered = $output;
          $rows[] = $node;
          unset($node);
        }

        date_modify($next, '+1 second');
        $now = date_format($next, DATE_FORMAT_DATE);  
        $pos++;    

      }
    }
    return $rows;
  }
}

