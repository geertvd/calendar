The module includes plug-in themes for Views to display available date fields
in calendar displays. To use them, select one of the calendar displays as the 
page type for your view and add a Calendar: Start Date field to the view,
selecting the date field you want the calendar to display in the calendar.

Arguments can be confusing but are very powerful. You can combine arguments in various 
ways, like year/month, year/month/day, or year/week. You can also add content type 
and taxonomy arguments before or after the year/month/day arguments for even more 
granularity. For each argument you add, you need to select a display. For lower-level 
arguments, like the year argument in a year/month combination, select the option to 
provide a summary view. That will display a list of all the months that have events 
in that year. You can then click on the month to see the events for that month.

To view a month calendar in views, you would set up arguments for year and month, then go 
to the url YYYY/MM to view that month. If you add a day argument to the view, set it up to display 
as a calendar month, then go to YYYY/MM/DD you will see a month calendar that only has the 
one day in it, since the /DD argument tells views to filter out just that one day. 

To view a week calendar, create a view with Year and Week arguments, the go to YYYY/WW
where WW is the week number (1-53) that you want to see.






