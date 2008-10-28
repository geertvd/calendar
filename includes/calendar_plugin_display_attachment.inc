<?php
// $Id$
/**
 * The plugin that handles calendar attachment displays.
 * 
 * Adding year/month/day/week pages as attachments makes it
 * possible to use any style type, so they could be tables,
 * lists, teasers, or nodes as well as traditional calendar
 * pages.
 * 
 * Force 'inherit_arguments' and 'inherit_filters' to TRUE, 
 * and 'attachment_position' to 'after', and don't display 
 * those options in the UI.
 * 
 * Allows paging (regular attachments do not), and adds an option 
 * to choose what calendar period this display represents.
 */
class calendar_plugin_display_attachment extends views_plugin_display_attachment {

  /**
   * Instead of going through the standard views_view.tpl.php, delegate this
   * to the style handler.
   */
  function render() {
    return $this->view->style_plugin->render($this->view->result);
  }
  
  /**
   * Create an array of possible display periods.
   */
  function display_types($type = 'month') {
    $types = calendar_display_types();
    return $types[$type];
  }

  /**
   * Identify the period of this display.
   */
  function calendar_type() {
    $types = calendar_display_types();
    $default = $this->get_option('calendar_type');
    if (!array_key_exists($default, $types)) $default = 'month';
    return $default;    
  }
  
  /**
   * Display validation.
   */
  function validate() {
    $errors = parent::validate();
    
    $arguments = $this->display->handler->get_option('arguments');
    if (!in_array('date_argument', array_keys($arguments))) {
      $errors[] = t("The Calendar period display '@display_title' will not work without a Date argument.", array('@display_title' => $this->definition['title']));      
    }
    elseif ($arguments['date_argument']['default_action'] != 'default' || $arguments['date_argument']['default_argument_type'] != 'date') {
      $errors[] = t("The Date argument @title in this view must be set up to provide a default value set to the current date.", array('@title' => $arguments['date_argument']['title']));      
    }
    return $errors;
  }
  
  /**
   * Attach only the appropriate displays for the current argument.
   */
  function attach_to($display_id) {
    // Inspect argument and view information to see which calendar
    // period we should show. The argument tells us what to use
    // if there is no value, the view args tell us what to use
    // if there are values.
    
    $display_granularity = $this->calendar_type();
    $arguments = $this->view->get_items('argument', $display_id);
    $wildcard = '';
    $argument = '';
    $default_granularity = '';
    $i = 0;
    foreach ($arguments as $argument) {
      if ($argument['id'] == 'date_argument') {
        $pos = $i;
        $default_granularity = $argument['granularity'];
        $wildcard = $argument['wildcard'];
        $argument = !empty($this->view->args) && !empty($this->view->args[$pos]) ? $this->view->args[$pos] : '';
        break;
      }
      $i++;
    }
    // TODO Anything else we need to do for 'all' arguments?
    if ($argument == $wildcard) {
      $view_granularity = $default_granularity;
    }
    elseif (!empty($argument)) {
      require_once('./'. drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
      $date_handler = new date_sql_handler();
      $view_granularity = $date_handler->arg_granularity($argument);
    }
    else {
      $view_granularity = $default_granularity;
    }
    
    // If this is not the right display to show, 
    // don't attach it, just exit.
    if ($view_granularity != $display_granularity) {
      return;
    }
    
    // See if we're attaching to a block rather than a page.
    $this->view->parent_id = $display_id;
    if (substr($display_id, 0, 14) == 'calendar_block') {
      $this->view->mini = TRUE;
      $this->view->block = TRUE;
      $this->calendar_popup = FALSE;
      if (!isset($this->view->block_identifier)) {
        $this->view->block_identifier = 'mini';
      }
    }
    elseif (substr($display_id, 0, 8) == 'calendar') {
      $this->view->calendar_colors = $this->view->display[$display_id]->handler->options['calendar_colors'];
      $this->view->calendar_popup = $this->view->display[$display_id]->handler->options['calendar_popup'];
    }
    $this->view->display_granularity = $display_granularity;
    $this->view->calendar_type = $this->calendar_type();
    parent::attach_to($display_id);
  }
  
  /**
   * Set default values for the display.
   */
  function options(&$display) {
    parent::options($display);
    $display->display_options['inherit_argments'] = TRUE;
    $display->display_options['inherit_filters'] = TRUE;
    $display->display_options['attachment_position'] = 'after';
    $display->display_options['calendar_type'] = $this->calendar_type();
  }  

  /**
   * Add custom option definitions.
   */
  function option_definition () {
    $options = parent::option_definition();
    $options['calendar_type'] = $this->calendar_type();
    return $options;
  }
  
 function options_form(&$form, &$form_state) {
    // It is very important to call the parent function here:
    parent::options_form($form, $form_state);
       
    switch ($form_state['section']) {
      case 'calendar_type':
        $form['#title'] .= t('Calendar period');
        $form['calendar_type'] = array(
          '#type' => 'select',
          '#description' => t('Select the calendar time period for this display.'),
          '#default_value' => $this->calendar_type(),
          '#options' => calendar_display_types(),
          );
         break;
    }
  }

  /**
   * Perform any necessary changes to the form values prior to storage.
   * There is no need for this function to actually store the data.
   */
  function options_submit($form, &$form_state) {
    // It is very important to call the parent function here:
    parent::options_submit($form, $form_state);
    switch ($form_state['section']) {
      case 'calendar_type':
        $this->set_option($form_state['section'], $form_state['values'][$form_state['section']]);
        break;
    }
  }
    
  /**
   * Provide the summary for attachment options in the views UI.
   *
   * This output is returned as an array.
   */
  function options_summary(&$categories, &$options) {
    parent::options_summary($categories, $options);
    $types = calendar_display_types();
    $categories['calendar_settings'] = array(
      'title' => t('Calendar settings'),
    );
    
    $options['calendar_type'] = array(
      'category' => 'calendar_settings',
      'title' => t('Calendar period'),
      'value' => $types[$this->calendar_type()],
    );
  }

  /**
   * Take away the option to change these values.
   */
  function defaultable_sections($section = NULL) {
    if (in_array($section, array('inherit_argments', 'inherit_filters', 'attachment_position',))) {
      return FALSE;
    }
    return parent::defaultable_sections($section);
  }

}