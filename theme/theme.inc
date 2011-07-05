<?php

/**
 * @file
 * Theme functions for the Calendar module.
 */

/**
 * Preprocess an RSS feed
 */
function template_preprocess_calendar_style(&$vars) {
  global $base_url;
  global $language;

}

/**
 * Default theme function for all iCal rows.
 */
function template_preprocess_calendar_row_node(&$vars) {
  $view     = &$vars['view'];
  $options  = &$vars['options'];
  $item     = &$vars['row'];

}


/** @} End of addtogroup themeable */
