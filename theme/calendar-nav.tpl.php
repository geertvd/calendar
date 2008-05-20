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
 * $colspan_prev
 * $colspan_next
 *   The colspan needed for the prev and next navigation url, 
 *   if empty, there is no navigation link.
 * $colspan_middle
 *   The colspan needed for the navigation title.
 * $mini
 *   Whether or not this is a mini calendar.
 * $view
 *   The view object for this calendar.
 */
?>
<tr>
  <?php if (!empty($colspan_prev)) : ?>
    <th colspan="<?php print $colspan_prev; ?>" class="prev">
      <span class="next"> <?php print l(!$mini ? t('prev') : '' .' »', $prev_url); ?></span>
    </th>
  <?php endif; ?>
  <th colspan="<?php print $colspan_middle; ?>" class="heading">
    <?php print $nav_title ?>
  </th>
  <?php if (!empty($colspan_next)) : ?>
    <th colspan="<?php print $colspan_next; ?>" class="next">
      <span class="next"> <?php print l(!$mini ? t('next') : '' .' »', $next_url); ?></span>
    </th>
  <?php endif; ?>  
</tr>
