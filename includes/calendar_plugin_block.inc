<?php
// $Id$

class calendar_plugin_block extends calendar_plugin_display_attachment {
  function options(&$display) {
    parent::options($display);
    $display->display_options['calendar_type'] = 'month';
  }  
}