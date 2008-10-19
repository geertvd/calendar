<?php
// $Id$
/**
 * @file
 * Template to display a summary of the days items as a calendar month node.
 * 
 * 
 * @see template_preprocess_calendar_month_multiple_node.
 */ 
?>
<div class="view-item view-item-<?php print $view->name ?>">
  <div class="calendar monthview" id="<?php print $node->date_id ?>">
    <?php foreach ($types as $type): ?>
      <?php print theme('calendar_stripe_stripe', $type); ?>
    <?php endforeach; ?>
    <div class="view-item <?php print views_css_safe('view-item-'. $view->name) ?>">
      <div class="multiple-events"> 
        <?php print l(t('Click to see all @count events', array('@count' => $count)), $link) ?>
      </div>    
    </div>
  </div>    
</div>
