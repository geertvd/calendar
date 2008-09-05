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
 * Force 'inherit_arguments' to TRUE, and 'attachment_position' 
 * to 'after', and don't display those options in the UI.
 * 
 * Allows paging (regular attachments do not), and adds an option 
 * to choose what type of display this represents.
 */
class calendar_plugin_display_attachment extends views_plugin_display_attachment {

  /**
   * Attach only the appropriate displays for the current argument.
   */
  function attach_to($display_id) {
    $i = 0;
    $display_granularity = str_replace('calendar_plugin_', '', $this->definition['handler']);
    $arguments = $this->view->get_items('argument', $display_id);
    foreach ($arguments as $argument) {
      if ($argument['id'] == 'date_argument') {
        $pos = $i;
        $default_granularity = $argument['granularity'];
        break;
      }
      $i++;
    }
    if (!empty($this->view->args[$pos])) {
      require_once('./'. drupal_get_path('module', 'date_api') .'/date_api_sql.inc');
      $date_handler = new date_sql_handler();
      $view_granularity = $date_handler->arg_granularity($this->view->args[$pos]);
    }
    else {
      $view_granularity = $default_granularity;
    }
    
    // See if we're attaching to a block rather than a page.
    if (substr($display_id, 0, 14) == 'calendar_block') {
      $this->view->mini = TRUE;
      $this->view->block = TRUE;
      $this->view->parent_id = $display_id;
    }
    elseif (substr($display_id, 0, 8) == 'calendar') {
      $this->view->parent_id = $display_id;
      $this->view->calendar_colors = $this->view->display[$display_id]->handler->options['calendar_colors'];
      if ($view_granularity != $display_granularity) {
        return;
      }
    }
    $this->view->display_granularity = $display_granularity;
    parent::attach_to($display_id);
    
  }
  
  function display_types($type = 'month') {
    $types = calendar_display_types();
    return $types[$type];
  }

  function options(&$display) {
    parent::options($display);
    $display->display_options['inherit_argments'] = TRUE;
    $display->display_options['attachment_position'] = 'after';
    $display->display_options['calendar_type'] = 'month';
  }  

  /**
   * Provide the summary for attachment options in the views UI.
   *
   * This output is returned as an array.
   */
  function options_summary(&$categories, &$options) {
    parent::options_summary($categories, $options);
    unset($options['attachment_position'], $options['inherit_arguments']);
  }
    
  function defaultable_sections($section = NULL) {
    if (in_array($section, array('inherit_argments', 'attachment_position',))) {
      return FALSE;
    }
    return parent::defaultable_sections($section);
  }
  
}

class calendar_plugin_year extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'year';
  }  
}

class calendar_plugin_month extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'month';
  }  
}

class calendar_plugin_day extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'day';
  }  
}

class calendar_plugin_week extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'week';
  }  
}

class calendar_plugin_block extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'month';
  }  
}