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
 * $mini: Whether or not this is a mini calendar.
 * $block: Whether or not this calendar is in a block.
 * $view
 *   The view object for this calendar.
 * 
 * The &nbsp; in the prev and next divs is to be sure they are never
 * completely empty, needed in some browsers to prop the header open
 * so the title stays centered.
 * 
 */
?>
<div class="calendar-nav clear-block">
  <div class="prev">
    <?php if (!empty($prev_url)) : ?>
      <span class="next"> <?php print l($mini ? '«' : t('« prev'), $prev_url); ?></span>
    <?php endif; ?>
  &nbsp;</div>
  <div class="heading">
    <h3><?php print $nav_title ?></h3>
  </div>
  <div class="next">&nbsp;
    <?php if (!empty($next_url)) : ?>
      <span class="next"> <?php print l($mini ? '»' : t('next »'), $next_url); ?></span>
    <?php endif; ?>  
  </div>
</div>
