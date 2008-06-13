<?php
//$Id$
//views_include_handlers();
/**
 * Implementation of hook_views_plugins
 */
function calendar_ical_views_plugins() {
  $path = drupal_get_path('module', 'calendar');
  require_once "./$path/theme/theme.inc";

  $base = array(
    'file' => 'theme.inc',
    'path' => "$path/theme",
  );
  return array(
    'module' => 'calendar_ical', // This just tells our themes are elsewhere.
    'display' => array(
      'ical' => $base + array(
        'title' => t('iCal feed'),
        'help' => t('Display the view as an iCal feed.'),
        'handler' => 'calendar_plugin_display_ical',
        'uses hook menu' => TRUE,
        'theme' => 'views_view',
        'no ui' => TRUE,
        'no remove' => TRUE,
        'use ajax' => FALSE,
        'use pager' => FALSE,
        'accept attachments' => FALSE,
        'admin' => t('iCal feed'),
        'help topic' => 'display-ical',
      ),
    ),
    'style' => array(
      'ical' => $base + array(
      'title' => t('iCal Feed'),
      'help' => t('Generates an iCal feed from a view.'),
      'handler' => 'calendar_plugin_style_ical',
      'theme' => 'calendar_view_ical',
      'uses row plugin' => FALSE,
      'uses fields' => TRUE,
      'uses row plugin' => FALSE,
      'uses options' => TRUE,
      'type' => 'ical',
      'even empty' => TRUE,
      ),
    ),
  );
}

/**
 * The plugin that handles a feed, such as RSS or atom.
 *
 * For the most part, feeds are page displays but with some subtle differences.
 */
class calendar_plugin_display_ical extends views_plugin_display_feed {
  function get_style_type() { return 'ical'; }

  /**
   * Provide the default form for setting options.
   */
  function options_form(&$form, &$form_state) {
    // It is very important to call the parent function here.
    parent::options_form($form, $form_state);

    switch ($form_state['section']) {
      case 'path':
        $form['path']['#description'] = t('This view will be displayed by visiting this path on your site. It is recommended that the path be something like "path/%/%/ical", putting one % in the path for each argument you have defined in the view.');
    }
  }

  /**
   * Instead of going through the standard views_view.tpl.php, delegate this
   * to the style handler.
   */
  function render() {
    return $this->view->style_plugin->render($this->view->result);
  }
}

/**
 * Default style plugin to render an iCal feed.
 */
class calendar_plugin_style_ical extends views_plugin_style_rss {
  function init(&$view, &$display) {
    parent::init($view, $display);
    $fields = $display->handler->default_display->options['fields'];
    $this->options['fields'] = $fields;
  }
  
  function query() {
    // We need these values for the iCal feed.
    $this->view->query->add_field('node', 'title');
    $this->view->query->add_field('node', 'type');
    parent::query();
  }
  
  function attach_to($display_id, $path, $title) {
    $display = $this->view->display[$display_id]->handler;
    $url_options = array();
    if (!empty($view->exposed_raw_input)) {
      $url_options['query'] = $view->exposed_raw_input;
    }

    $url = url($this->view->get_url(NULL, $path), $url_options);
    if (!$display->has_path()) {
      if (empty($this->preview)) {
        calendar_ical_add_feed($url, $title);
      }
    }
    else {
      if (empty($this->view->feed_icon)) {
        $this->view->feed_icon = '';
      }
      
      $this->view->feed_icon .= theme('calendar_ical_icon', $url, $title);
    }
  }

  /**
   * Set default options
   */
  function options(&$options) {
    parent::options($options);
    $options['summary_field'] = 'node_title';
    $options['description_field'] = '';
    $options['location_field'] = '';
    $options['fields'] = array();
  }
  
  function option_definition() {
    $options = parent::option_definition();

    $options['summary_field'] = array('default' => '', 'translatable' => TRUE);
    $options['description_field'] = array('default' => '', 'translatable' => TRUE);
    $options['location_field'] = array('default' => '', 'translatable' => TRUE);
    
    return $options;
  }

  function options_form(&$form, &$form_state) {
    $options = array('' => '');
    foreach ($this->options['fields'] as $field) {
      $handler = views_get_handler($field['table'], $field['field'], 'field');
      $alias = $field['table'] .'_'. $field['field'];
      $options[$alias] = $handler->ui_name();
    }
    $form['#prefix'] = t('<div class="form-item">Map the View fields to the values they should represent in the iCal feed.</div>');
    
    $form['summary_field'] = array(
      '#type' => 'select',
      '#title' => t('Title'),
      '#default_value' => !empty($this->options['summary_field']) ? $this->options['summary_field'] : 'node_title',
      '#options' => $options,
      );
    $form['description_field'] = array(
      '#type' => 'select',
      '#title' => t('Description'),
      '#default_value' => $this->options['description_field'],
      '#options' => $options,
      );
    $form['location_field'] = array(
      '#type' => 'select',
      '#title' => t('Location'),
      '#default_value' => $this->options['location_field'],
      '#options' => $options,
      );
      
  }

  function render() {
    include_once(drupal_get_path('module', 'calendar') .'/calendar.inc');
    
    // Transfer the style options to the view object so they
    // can be easily accessed in the theme.
    $style_options = $this->options;
    $this->view->summary_field = $style_options['summary_field'];
    $this->view->description_field = $style_options['description_field'];
    $this->view->location_field = $style_options['location_field'];
    
    // Evaluate our argument values and figure out which 
    // calendar display we need to create.
    $i = 0;   
    foreach ($this->view->argument as $id => $argument) {
      if ($id == 'date_argument') {
        $this->view->granularity = !empty($argument->granularity) ? $argument->granularity : $argument->options['granularity'];
        $this->view->date_arg = !empty($this->view->args) ? $this->view->args[$argument->position] : '';
        $this->view->date_arg_pos = $i;
        $this->view->year = isset($argument->year) ? $argument->year : NULL;
        $this->view->month = isset($argument->month) ? $argument->month: NULL;
        $this->view->day = isset($argument->day) ? $argument->day : NULL;
        $this->view->week = isset($argument->week) ? $argument->week : NULL;
        $this->view->min_date = $argument->min_date;
        $this->view->max_date = $argument->max_date;
      }
      $i++;
    }
    
    // Render each field into an output array.
    $items = array();
    $calendar_fields = date_api_fields();
    $calendar_fields = array_keys($calendar_fields['alias']);
    foreach ($this->view->result as $num => $row) {
      $items[$num] = $row;
      // Store the raw date values before formatting the results.
      foreach ($row as $key => $value) {
        if (in_array($key, $calendar_fields)) {
          $items[$num]->calendar_fields->$key = $value;
        }
      }
      foreach ($this->view->field as $field) {
        if (!empty($field) && is_object($field)) {
          $field_output = $field->theme($row);
          $items[$num]->{$field->field_alias} = $field_output;
        }
      }
    }

    // Massage the resulting items into formatted calendar items.
    $items = calendar_build_nodes($this->view, $items);
    
    // Merge in items from other sources.
    foreach (module_implements('calendar_add_items') as $module) {
      $function = $module .'_calendar_add_items';
      if (function_exists($function)) {
        if ($feeds = $function($this->view)) {
          foreach ($feeds as $feed) {
            $items = $feed;
          }
        }
      }
    }
    return theme($this->theme_functions(), $this->view, $this->options, $items);
  }
}
