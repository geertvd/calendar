/**
 * Implementation of hook_elements.
 *
 * Much of the colorpicker code was adapted from the Colorpicker module.
 * That module has no stable release yet nor any D6 branch.
 */
/*
 *  Bind the colorpicker event to the form element
 */
(function ($) {
  "use strict";

  Drupal.behaviors.calendar_colorpicker = {
    attach: function(context) {
      $(".edit-calendar-colorpicker").on("focus", function() {
        var edit_field = this;
        var picker = $(this).closest('div').parent().find(".calendar-colorpicker");

        // Hide all color pickers except this one.
        $(".calendar-colorpicker").hide();
        $(picker).show();
        $.farbtastic(picker, function(color) {
          edit_field.value = color;
        }).setColor(edit_field.value);
      });
    }
  }
})(jQuery);

