<?php
// $Id$
/**
 * @file
 * Template to display calendar naviagion links.
 *
 * - $links: Array of formatted links to other calendar displays, i.e. day, year.
 * - $calendar_nav: Formatted back/next navigation links.
 *     @see calendar-nav.tpl.php.
 * 
 * - $day_names: An array of the day of week names for the table header.
 * - $rows: An array of data for each day of the week.
 */
?>

<tr>
  <th colspan="<?php print $colspan_prev; ?>" class="views-field views-field-<?php print $data['class']; ?>">
    <span class="next"> <?php print l(!$mini ? t('prev') : '' .' »', $prev_url); ?></span>
  </th>
  <th colspan="<?php print $colspan_middle; ?>" class="views-field views-field-<?php print $data['class']; ?>">
    <?php print $nav_title ?>
  </th>
  <th colspan="<?php print $colspan_next; ?>" class="views-field views-field-<?php print $data['class']; ?>">
    <span class="next"> <?php print l(!$mini ? t('next') : '' .' »', $next_url); ?></span>
  </th>
</tr>
