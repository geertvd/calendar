<?php
// $Id$
/**
 * @file
 * Template to display a view as a calendar month.
 * 
 * @see template_preprocess_calendar_month.
 *
 * - $view: The view.
 * - $links: Array of formatted links to other calendar displays, i.e. day, year.
 * - $day_names: An array of the day of week names for the table header.
 * - $rows: An array of data for each day of the week.
 * - $calendar_nav: Formatted back/next navigation links.
 *     @see calendar-nav.tpl.php.
 * 
 */
dsm('Display: '. $display_type .': '. $min_date_formatted .' to '. $max_date_formatted);

?>

<div class="calendar-calendar"><div class="year-view">

<?php print theme('links', $calendar_links);?>

<table <?php if ($mini): ?> class="mini"<? endif; ?>>
  <thead>
    <?php print $calendar_nav ?>
  </thead>
  <tbody>
    <tr><td><?php print $months[1] ?></td><td><?php print $months[2] ?></td><td><?php print $months[3] ?></td></tr>  
    <tr><td><?php print $months[4] ?></td><td><?php print $months[5] ?></td><td><?php print $months[6] ?></td></tr>  
    <tr><td><?php print $months[7] ?></td><td><?php print $months[8] ?></td><td><?php print $months[9] ?></td></tr>  
    <tr><td><?php print $months[10] ?></td><td><?php print $months[11] ?></td><td><?php print $months[12] ?></td></tr>  
  </tbody>
</table>
</div></div>