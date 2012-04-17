<?php
/**
 * @file
 * Template to display a column
 * 
 * - $item: The item to render within a td element.
 */
$id = (isset($item['id'])) ? 'id="' . $item['id'] . '" ' : '';
$date = (isset($item['date'])) ? ' date="' . $item['date'] . '" ' : '';
$axis = (isset($item['axis'])) ? ' axis="'. $item['axis'] .'" ' : '';
?>
<td <?php print $id?>class="<?php print $item['class'] ?>" colspan="<?php print $item['colspan'] ?>" rowspan="<?php print $item['rowspan'] ?>"<?php print $date ?><?php print $axis; ?>>
  <div class="inner">
    <?php print $item['entry'] ?>
  </div>
</td>
