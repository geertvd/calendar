<?php

/**
 * @file
 * Views style plugin for the Calendar iCal module.
 */

/**
 * Default style plugin to render an iCal feed.
 */
class calendar_plugin_style extends views_plugin_style {

  function init(&$view, &$display, $options = NULL) {
    parent::init($view, $display, $options);
    if (empty($this->view->date_info)) {
      $this->view->date_info = new stdClass();
    }
  }

  /**
   * Add custom option definitions.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['calendar_type'] = array('default' => 'month');
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
      $form['#title'] .= t('Calendar period');
      $form['calendar_type'] = array(
        '#type' => 'select',
        '#description' => t('Select the calendar time period for this display.'),
        '#default_value' => $this->option['calendar_type'],
        '#options' => calendar_display_types(),
      );
  }

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

  /**
   * Inspect argument and view information to see which calendar
   * period we should show. The argument tells us what to use
   * if there is no value, the view args tell us what to use
   * if there are values.
   */
  function granularity() {

    $default_granularity = '';
    $handler = $this->date_argument_handler();
    $default_granularity = !empty($handler) ? $handler->granularity : 'month';
    $wildcard = !empty($handler) ? $handler->options['exception']['value'] : '';
    $argument = $handler->argument;

    // TODO Anything else we need to do for 'all' arguments?
    if ($argument == $wildcard) {
      $view_granularity = $default_granularity;
    }
    elseif (!empty($argument)) {
      module_load_include('inc', 'date_api', 'date_api_sql');

      $date_handler = new date_sql_handler();
      $view_granularity = $date_handler->arg_granularity($argument);
    }
    else {
      $view_granularity = $default_granularity;
    }
    return $view_granularity;
  }

  function render() {
    if (empty($this->row_plugin)) {
      debug('views_plugin_style_default: Missing row plugin');
      return;
    }

    // Add information from the date argument to the view.
    $argument = $this->date_argument_handler();
    $this->view->date_info->granularity = $argument->granularity;
    $this->view->date_info->date_arg = $argument->argument;
    $this->view->date_info->year = date_format($argument->min_date, 'Y');
    $this->view->date_info->month = date_format($argument->min_date, 'n');;
    $this->view->date_info->day = date_format($argument->min_date, 'j');
    $this->view->date_info->week = date_week(date_format($argument->min_date, DATE_FORMAT_DATE));
    $this->view->date_info->date_range = $argument->date_range;
    $this->view->date_info->min_date = $argument->min_date;
    $this->view->date_info->max_date = $argument->max_date;
    $this->view->date_info->limit = $argument->limit;
    $this->view->date_info->url = $this->view->get_url();
    $this->view->date_info->min_date_date = date_format($this->view->date_info->min_date, DATE_FORMAT_DATE);
    $this->view->date_info->max_date_date = date_format($this->view->date_info->max_date, DATE_FORMAT_DATE);
    $this->view->date_info->forbid = isset($argument->forbid) ? $argument->forbid : FALSE;

    $rows = '';
    foreach ($this->view->result as $row_index => $row) {
      $this->view->row_index = $row_index;
      $rows .= $this->row_plugin->render($row);
    }

    $output = theme($this->theme_functions(),
      array(
        'view' => $this->view,
        'options' => $this->options,
        'rows' => $rows
      ));
    unset($this->view->row_index);
    return $output;
  }

}