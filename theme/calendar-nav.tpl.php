<?php
// $Id$
/**
 * @file
 * Template to display calendar naviagion links.
 *
 */
?>

<tr>
  <th colspan="<?php print $colspan_prev; ?>" class="prev">
    <span class="next"> <?php print l(!$mini ? t('prev') : '' .' »', $prev_url); ?></span>
  </th>
  <th colspan="<?php print $colspan_middle; ?>" class="heading">
    <?php print $nav_title ?>
  </th>
  <th colspan="<?php print $colspan_next; ?>" class="next">
    <span class="next"> <?php print l(!$mini ? t('next') : '' .' »', $next_url); ?></span>
  </th>
</tr>
