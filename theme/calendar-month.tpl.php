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

<div class="calendar-calendar"><div class="month-view">

<?php if (empty($mini)) print theme('links', $calendar_links);?>

<table <?php if ($mini): ?> class="mini"<? endif; ?>>
  <thead>
    <?php print $calendar_nav ?>
    <tr>
      <?php foreach ($day_names as $cell): ?>
        <th id="<?php print $cell['id']; ?>" class="views-field <?php print $cell['class']; ?>">
          <?php print $cell['data']; ?>
        </th>
      <?php endforeach; ?>
    </tr>
  </thead>
  <tbody>
    <?php foreach ($rows as $row): ?>
      <tr>
        <?php foreach ($row as $cell): ?>
          <td id="<?php print $cell['id']; ?>" class="views-field <?php print $cell['class']; ?>">
            <?php print $cell['data']; ?>
          </td>
        <?php endforeach; ?>
      </tr>
    <?php endforeach; ?>
  </tbody>
</table>
</div></div>