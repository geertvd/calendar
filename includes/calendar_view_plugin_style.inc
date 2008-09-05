<?php
// $Id$
/**
 * Style plugin to render the year, month, week, or day calendar view.
 */
class calendar_view_plugin_style extends calendar_plugin_style {

  /**
   * Init will be called after construct, when the plugin is attached to a
   * view and a display. 
   */
  function init(&$view, &$display) {
    parent::init($view, $display);
    $view->name_size = $this->options['name_size'];
    $view->with_weekno = $this->options['with_weekno'];
  }
  
  /**
   * Set default options
   */
  function options(&$options) {
    $options['name_size'] = 3;
    $options['with_weekno'] = 0;
  }

  /**
   * Style options.
   */
  function options_form(&$form, &$form_state) {
    $form['name_size'] = array(
      '#title' => t('Calendar day of week names'),
      '#default_value' => $this->options['name_size'],
      '#type' => 'radios',
      '#options' => array(1 => t('First letter of name'), 2 => t('First two letters of name'), 3 => t('Abbreviated name'), 99 => t('Full name')),
      '#description' => t('The way day of week names should be displayed in a calendar.'),
      );
    $form['with_weekno'] = array(
      '#title' => t('Show week numbers'),
      '#default_value' => $this->options['with_weekno'],
      '#type' => 'radios',
      '#options' => array(0 => t('No'), 1 => t('Yes')),
      '#description' => t('Whether or not to show week numbers in the left column of calendar weeks and months.'),
      );

  }
  
   /**
   * Render the calendar attachment style.
   */
  function render() {
    // Adjust the theme to match the currently selected default.
    $this->definition['theme'] = 'calendar_'. $this->view->granularity;
    $this->view->hide_admin_links = TRUE;
    return theme($this->theme_functions(), $this->view, $this->options, array());
  }
}