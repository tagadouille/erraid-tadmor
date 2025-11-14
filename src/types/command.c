#include "types/command.h"

// ! A tester
void command_execute(const command_t *cmd)
{
    if (cmd == NULL) // Security check
        return;

    if (cmd->type == SI)
    {
        // Execute simple command
        if (cmd->u.simple.argc > 0) // On vérifie qu'il y a au moins un argument
        {
                // nom de la commande    // arguments
            execvp(cmd->u.simple.argv[0], cmd->u.simple.argv + 1);
            perror("execvp failed"); // Si execvp échoue, on affiche une erreur
        }
    }
    else // SQ
    {
        // Execute composed commands sequentially
        for (uint32_t i = 1; i < cmd->u.composed.count; i++)
        {
            command_execute(cmd->u.composed.cmds[i]); // Appel récursif
        }
    }
    command_free((command_t *)cmd); // Free the command after execution
}

void command_free(command_t *cmd)
{
    if (cmd == NULL)
        return;

    if (cmd->type == SQ)
    {
        // Free each sub-command
        for (uint32_t i = 0; i < cmd->u.composed.count; i++)
        {
            command_free(cmd->u.composed.cmds[i]);
        }
        free(cmd->u.composed.cmds);
    }
    else // SI
    {
        arguments_free(&cmd->u.simple);
    }
    free(cmd);
}