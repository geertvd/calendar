<?php
// $Id$
/**
 * @file
 * Template to display a view as a calendar day.
 * 
 * @see template_preprocess_calendar_day.
 *
 * $day_names: The day of week info for the table header.
 * $rows: An array of data for this day.
 * $view: The view.
 * $min_date_formatted: The minimum date for this calendar in the format YYYY-MM-DD HH:MM:SS.
 * $max_date_formatted: The maximum date for this calendar in the format YYYY-MM-DD HH:MM:SS.
 */
//dsm('Display: '. $display_type .': '. $min_date_formatted .' to '. $max_date_formatted);
?>

<div class="calendar-calendar"><div class="day-view">
<table>
  <thead>
    <tr>
      <th colspan="3">
        <?php print $day_names; ?>
      </th>
    </tr>
  </thead>
  <tbody>
    <?php foreach ((array) $rows as $row): ?>
      <tr>
        <?php foreach ($row as $cell): ?>
          <td id="<?php print $cell['id']; ?>" class="<?php print $cell['class']; ?>">
            <?php print $cell['data']; ?>
          </td>
        <?php endforeach; ?>
      </tr>
    <?php endforeach; ?>
  </tbody>
</table>
</div></div>