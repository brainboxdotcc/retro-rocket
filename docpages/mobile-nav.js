document.addEventListener("DOMContentLoaded", function () {
    const toggleLabel = document.querySelector("label.main-menu-btn");
    const toggleInput = document.getElementById("main-menu-state");
    const sideNav = document.getElementById("side-nav");

    function toggleSideNav(e) {
    if (e) {
    e.preventDefault();
    e.stopPropagation();
}

    if (!sideNav) {
    return;
}

    sideNav.classList.toggle("open");
}

    if (toggleLabel) {
    toggleLabel.addEventListener("click", toggleSideNav);
}

    if (toggleInput) {
    toggleInput.addEventListener("change", function () {
    if (!sideNav) {
    return;
}

    sideNav.classList.toggle("open", toggleInput.checked);
});
}
});
