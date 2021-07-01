$(window).on("scroll", function() {
    if ($(window).scrollTop()) {
        $('div.header').addClass('shadow');
        $('div.head_inner').addClass('blackTextBar a');
    } else {
        $('div.header').removeClass('shadow');
        $('div.head_inner').removeClass('blackTextBar a');
    }
});