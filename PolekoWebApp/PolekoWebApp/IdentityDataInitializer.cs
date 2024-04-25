using Microsoft.AspNetCore.Identity;
using PolekoWebApp.Data;

public static class IdentityDataInitializer
{
    private static readonly string[] Roles = ["SuperAdmin", "Admin", "User"];

    public static async Task SeedRoles(RoleManager<IdentityRole> roleManager)
    {
        foreach (var roleName in Roles)
        {
            if (await roleManager.RoleExistsAsync(roleName)) continue;
            var role = new IdentityRole { Name = roleName };
            await roleManager.CreateAsync(role);
        }
    }

    public static async Task RemoveUsers(UserManager<ApplicationUser> userManager)
    {
        var users = userManager.Users.ToList();
        foreach (var user in users) await userManager.DeleteAsync(user);
    }
}