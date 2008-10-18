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
    $view->style_name_size = $this->options['name_size'];
    $view->style_with_weekno = $this->options['with_weekno'];
    $view->style_limit = $this->options['limit'];
  }
  
  /**
   * Set default options
   */
  function options(&$options) {
    $options['name_size'] = 3;
    $options['with_weekno'] = 0;
    $options['limit'] = 5;
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
    $form['limit'] = array(
      '#title' => t('Items to show'),
      '#type' => 'select',
      '#options' => array(0 => t('Unlimited'), 3 => t('3 items'), 5 => t('5 items'), 10 => t('10 items')),
      '#default_value' => $this->options['limit'],
      '#description' => t('Maximum number of items to show in calendar cells, used to keep the calendar from expanding to a huge size when there are lots of items in one day. When there more items than this limit, a summary message with a link to the day view will be displayed instead.'),
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