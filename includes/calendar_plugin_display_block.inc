<?php
// $Id$
/**
 * The plugin that handles a calendar block.
 * 
 * The only style option that will be available is the calendar
 * style, which creates the navigation and links to other calendar
 * displays. All options for paging, row plugins, etc. are
 * deferred to the attachments.
 */
class calendar_plugin_display_block extends views_plugin_display_block {
  /**
   * Init will be called after construct, when the plugin is attached to a
   * view and a display. 
   */
  function init(&$view, &$display) {
    parent::init($view, $display);
    if (!isset($display->display_options['style_options'])) {
      return;
    }
    if (!$granularity = calendar_current_type($view)) {
      $granularity = $display->display_options['style_options']['default_display'];
    }
    $view->granularity = $granularity;
    $view->default_display = $display->display_options['style_options']['default_display'];
    if ($display->id == 'calendar_block') {
      $view->mini = TRUE;
      $view->block = TRUE;
    }
  }
  
  function get_style_type() { return 'calendar'; }
  
  function defaultable_sections($section = NULL) {
    if (in_array($section, array('style_plugin', 'row_options', 'row_plugin', 'items_per_page'))) {
      return FALSE;
    }
    return parent::defaultable_sections($section);
  }

  function options(&$display) {
    parent::options($display);
    $display->display_options['displays'] = array();
    $display->display_options['style_plugin'] = 'calendar_nav';
    $display->display_options['items_per_page'] = 0;
    $display->display_options['row_plugin'] = '';
    $display->display_options['defaults']['style_plugin'] = FALSE;
    $display->display_options['defaults']['style_options'] = FALSE;
    $display->display_options['defaults']['items_per_page'] = FALSE;
    $display->display_options['defaults']['row_plugin'] = FALSE;
    $display->display_options['defaults']['row_options'] = FALSE;
  } 
  
  /**
   * The display block handler returns the structure necessary for a block.
   * 
   * TODO This can be removed when the patch at http://drupal.org/node/290328
   * gets into an official release.
   */
  function execute() {
    // Prior to this being called, the $view should already be set to this
    // display, and arguments should be set on the view.
    $info['content'] = $this->view->render();
    $info['subject'] = filter_xss_admin($this->view->get_title());
    if (!empty($this->view->result) || $this->get_option('empty') || !empty($this->view->style_plugin->definition['even empty'])) {
      return $info;
    }
  }

  function query() {
    // We don't need a query for the navigation. There's no way to
    // completely turn it off, but we can set it to something simple.
    $this->view->query->clear_fields();
    $this->view->query->where = array();
    $this->view->query->add_where(NULL, "0");
  }
  
}
