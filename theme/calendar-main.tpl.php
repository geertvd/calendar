<?php
// $Id$
/**
 * @file
 * Template to display calendar navigation and links.
 * 
 * @see template_preprocess_calendar_main.
 *
 * $view: The view.
 * $calendar_links: Array of formatted links to other calendar displays - year, month, week, day.
 * $display_type: year, month, day, or week.
 * $mini: Whether this is a mini view.
 * $min_date_formatted: The minimum date for this calendar in the format YYYY-MM-DD HH:MM:SS.
 * $max_date_formatted: The maximum date for this calendar in the format YYYY-MM-DD HH:MM:SS.
 * 
 */
//dsm('Display: '. $display_type .': '. $min_date_formatted .' to '. $max_date_formatted);
?>

<div class="calendar-calendar">
  <?php if (empty($mini)) print theme('links', $calendar_links);?>
  <?php print theme('calendar_nav', $view) ?>
</div>