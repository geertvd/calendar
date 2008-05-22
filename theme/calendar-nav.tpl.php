<?php
// $Id$
/**
 * @file
 * Template to display calendar naviagion links.
 *
 * $nav_title
 *   The formatted title for this calendar. In the case of mini
 *   calendars, it will be a link to the full view of the calendar,
 *   otherwise it will be the formatted name of the year, month, day,
 *   or week.
 * $prev_url
 * $next_url
 *   Urls for the previous and next calendar pages. The links are 
 *   composed in the template to make it easier to change the text,
 *   add images, etc.
 * $mini
 *   Whether or not this is a mini calendar.
 * $view
 *   The view object for this calendar.
 */
?>
<div class="calendar-nav clear-block">
  <?php if (!empty($colspan_prev)) : ?>
    <div class="prev">
      <span class="next"> <?php print l($mini ? '«' : t('« prev'), $prev_url); ?></span>
    </div>
  <?php endif; ?>
  <div class="heading">
    <h3><?php print $nav_title ?></h3>
  </div>
  <?php if (!empty($colspan_next)) : ?>
    <div class="next">
      <span class="next"> <?php print l($mini ? '»' : t('next »'), $next_url); ?></span>
    </div>
  <?php endif; ?>  
</div>
