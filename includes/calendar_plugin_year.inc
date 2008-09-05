<?php
// $Id$

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