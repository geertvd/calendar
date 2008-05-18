<?php
// $Id$
/**
 * @file views-view-calendar.tpl.php
 * Template to display a view as a calendar.
 *
 * - $header: An array of header labels keyed by field id.
 * - $fields: An array of CSS IDs to use for each field id.
 * - $rows: An array of row items. Each row is an array of content
 *   keyed by field ID.
 * @ingroup views_templates
 */
dsm('Display: '. $display_type .': '. $min_date .' to '. $max_date);

//dsm($header);
//dsm('Formatted results');
//dsm($rows);
//dsm('Raw results');
//dsm($result);
//dsm('Items');
//dsm($items);
?>
<?php print $links ?>
<table class="<?php print $class; ?>">
  <thead>
    <tr>
      <?php foreach ($header as $data): ?>
        <th colspan="<?php print $data['colspan']; ?>" class="views-field views-field-<?php print $data['class']; ?>">
          <?php print $data['data']; ?>
        </th>
      <?php endforeach ?>
    </tr>
  </thead>
  <tbody>
    <?php foreach ($rows as $row): ?>
      <tr>
        <?php foreach ($row as $cell): ?>
          <td id="<?php print $cell['id']; ?> class="views-field views-field-<?php print $cell['class']; ?>">
            <?php print $cell['data']; ?>
          </td>
        <?php endforeach; ?>
      </tr>
    <?php endforeach; ?>
  </tbody>
</table>